/*
 * Copyright (C) 2016-2021 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2020 Veloman Yunkan
 * Copyright (C) 2009 Tommi Maekitalo
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * is provided AS IS, WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, and
 * NON-INFRINGEMENT.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#include "cluster.h"
#include <zim/blob.h>
#include <zim/error.h>
#include "buffer_reader.h"
#include "bufferstreamer.h"
#include "decoderstreamreader.h"
#include "rawstreamreader.h"
#include <algorithm>
#include <stdlib.h>

#include "log.h"

log_define("zim.cluster")

#define log_debug1(e)

namespace zim
{

namespace
{

std::unique_ptr<IStreamReader>
getClusterReader(const Reader& zimReader, offset_t offset, Cluster::Compression* comp, bool* extended)
{
  uint8_t clusterInfo = zimReader.read(offset);
  // Very old zim files used 0 as a "default" compression, which means no compression.
  uint8_t compInfo = clusterInfo & 0x0F;
  if(compInfo == 0) {
    *comp = Cluster::Compression::None;
  } else if (compInfo == int(Cluster::Compression::Zip)) {
    throw std::runtime_error("zlib not enabled in this library");
  } else if (compInfo == int(Cluster::Compression::Bzip2)) {
    throw std::runtime_error("bzip2 not enabled in this library");
  } else {
    *comp = static_cast<Cluster::Compression>(compInfo);
  }
  *extended = clusterInfo & 0x10;
  auto subReader = std::shared_ptr<const Reader>(zimReader.sub_reader(offset+offset_t(1)));

  switch ( *comp ) {
    case Cluster::Compression::None:
      return std::unique_ptr<IStreamReader>(new RawStreamReader(subReader));
    case Cluster::Compression::Lzma:
      return std::unique_ptr<IStreamReader>(new DecoderStreamReader<LZMA_INFO>(subReader));
    case Cluster::Compression::Zstd:
      return std::unique_ptr<IStreamReader>(new DecoderStreamReader<ZSTD_INFO>(subReader));
    default:
      throw ZimFileFormatError("Invalid compression flag");
  }
}

} // unnamed namespace

  std::shared_ptr<Cluster> Cluster::read(const Reader& zimReader, offset_t clusterOffset)
  {
    Compression comp;
    bool extended;
    auto reader = getClusterReader(zimReader, clusterOffset, &comp, &extended);
    return std::make_shared<Cluster>(std::move(reader), comp, extended);
  }

  Cluster::Cluster(std::unique_ptr<IStreamReader> reader_, Compression comp, bool isExtended)
    : compression(comp),
      isExtended(isExtended),
      m_reader(std::move(reader_)),
      m_memorySize(0)
  {
    if (isExtended) {
      read_header<uint64_t>();
    } else {
      read_header<uint32_t>();
    }
  }

  Cluster::~Cluster() = default;

  /* This return the number of char read */
  template<typename OFFSET_TYPE>
  void Cluster::read_header()
  {
    // read first offset, which specifies, how many offsets we need to read
    OFFSET_TYPE offset = m_reader->read<OFFSET_TYPE>();

    size_t n_offset = offset / sizeof(OFFSET_TYPE);

    // read offsets
    m_blobOffsets.clear();
    m_blobOffsets.reserve(n_offset);
    m_blobOffsets.push_back(offset_t(offset));

    // Get the whole offsets data to avoid to many (system) call.
    auto bufferSize = zsize_t(offset-sizeof(OFFSET_TYPE));
    auto buffer = m_reader->sub_reader(bufferSize)->get_buffer(offset_t(0), bufferSize);
    auto seqReader = BufferStreamer(buffer, bufferSize);
    while (--n_offset)
    {
      OFFSET_TYPE new_offset = seqReader.read<OFFSET_TYPE>();
      if (new_offset < offset) {
        throw zim::ZimFileFormatError("Error parsing cluster. Offsets are not ordered.");
      }

      m_blobOffsets.push_back(offset_t(new_offset));
      offset = new_offset;
    }
  }

  zsize_t Cluster::getBlobSize(blob_index_t n) const
  {
      if (blob_index_type(n)+1 >= m_blobOffsets.size()) {
        throw ZimFileFormatError("blob index out of range");
      }
      return zsize_t(m_blobOffsets[blob_index_type(n)+1].v - m_blobOffsets[blob_index_type(n)].v);
  }

  const Reader& Cluster::getReader(blob_index_t n) const
  {
    std::lock_guard<std::mutex> lock(m_readerAccessMutex);
    for(blob_index_type current(m_blobReaders.size()); current<=n.v; ++current) {
      auto blobSize = getBlobSize(blob_index_t(current));
      if (blobSize.v > SIZE_MAX) {
        m_blobReaders.push_back(std::unique_ptr<Reader>(new BufferReader(Buffer::makeBuffer(zsize_t(0)))));
      } else {
        m_blobReaders.push_back(m_reader->sub_reader(blobSize));
      }
    }
    return *m_blobReaders[blob_index_type(n)];
  }

  Blob Cluster::getBlob(blob_index_t n) const
  {
    if (n < count()) {
      const auto blobSize = getBlobSize(n);
      if (blobSize.v > SIZE_MAX) {
        return Blob();
      }
      return getReader(n).get_buffer(offset_t(0), blobSize);
    } else {
      return Blob();
    }
  }

  Blob Cluster::getBlob(blob_index_t n, offset_t offset, zsize_t size) const
  {
    if (n < count()) {
      const auto blobSize = getBlobSize(n);
      if ( offset.v > blobSize.v ) {
        return Blob();
      }
      size = std::min(size, zsize_t(blobSize.v-offset.v));
      if (size.v > SIZE_MAX) {
        return Blob();
      }
      return getReader(n).get_buffer(offset, size);
    } else {
      return Blob();
    }
  }

  // This function must return a constant size for a given cluster.
  // This is important as we want to remove the same size that what we add when we remove
  // the cluster from the cache.
  // However, because of partial decompression, this size can change:
  // - As we advance in the compression, we can create new blob readers in `m_blobReaders`
  // - The stream itself may allocate memory.
  // To solve this, we take the average and say a cluster's blob readers will half be created and
  // so we assume a readers size of half the full uncompressed cluster data size.
  // If cluster is not compressed, we never store its content (mmap is created on demand and not cached),
  // so we use a size of 0 for the readers.
  // It also appears that when we get the size of the stream, we reach a state where no
  // futher allocation will be done by it. Probably because:
  // - We already started to decompress the stream to read the offsets
  // - Cluster data size is smaller than window size associated to compression level (?)
  // We anyway check that and print a warning if this is not the case, hopping that user will create
  // an issue allowing us for further analysis.
  // Note:
  //  - No need to protect this method from concurent access as it will be called by the concurent_cache which will
  //    have a lock (on lru cache) to ensure only one thread access it in the same time.
  size_t Cluster::getMemorySize() const {
    if (!m_memorySize) {
      auto offsets_size = sizeof(offset_t) * m_blobOffsets.size();
      auto readers_size = 0;
      if (isCompressed()) {
        readers_size = m_blobOffsets.back().v / 2;
      }
      m_streamSize = m_reader->getMemorySize();
      // Compression level define a huge window and make decompression stream allocate a huge memory to store it.
      // However, the used memory will not be greater than the content itself, even if window is bigger.
      // On linux (at least), the real used memory will be the actual memory used, not the one allocated.
      // So, let's clamm the the stream size to the size of the content itself.
      m_memorySize = offsets_size + readers_size + std::min<size_type>(m_streamSize, m_blobOffsets.back().v);
    }
    auto streamSize = m_reader->getMemorySize();
    if (streamSize != m_streamSize) {
      std::cerr << "WARNING: stream size have changed from " << m_streamSize << " to " << streamSize << std::endl;
      std::cerr << "Please open an issue on https://github.com/openzim/libzim/issues with this message and the zim file you use" << std::endl;
    }
    return m_memorySize;
  }
}
