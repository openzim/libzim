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
      m_reader(std::move(reader_))
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

  // This function must return the memory consumption for a given cluster so
  // that it can be used as a cost estimate during caching.
  // However, because of partial (incremental) decompression, this size depends
  // on the state of decompression:
  // - As decompression advances, new blob readers are created in
  //   `m_blobReaders`
  // - The decoding/decompressing stream itself may allocate memory.
  // Our approach is to return the average memory consumption by this cluster
  // under the assumption that half of its data is decompressed.
  // Note:
  //  - No need to protect this method from concurent access (as well
  //    as memoize its result) as it is intended to be called by ConcurentCache
  //    which should invoke this method exactly once per cluster object
  size_t Cluster::getMemorySize() const {
    const auto blobOffsetsSize = sizeof(offset_t) * m_blobOffsets.size();
    const auto decompressedDataSize = m_blobOffsets.back().v;

    // If the cluster is not compressed, we rely on mmap and kernel
    // to do the memory management.
    const auto dataSize = isCompressed() ? decompressedDataSize : 0;

    // Memory consumption by the decompressor stream.
    // For non-compressed data reader it is assumed to be 0 (see the comment
    // in BaseFileReader::getMemorySize())
    auto streamSize = m_reader->getMemorySize();

    // Compression levels may define a huge window and make decompression
    // stream allocate a huge memory to store it. However (at least on
    // linux) the actual memory used (as opposed to allocated memory) will
    // not exceed the content size, even for a larger window. So, let's
    // clamp the stream size to the size of the content itself.
    streamSize = std::min<size_type>(streamSize, decompressedDataSize);

    return blobOffsetsSize + dataSize/2 + streamSize;
  }
}
