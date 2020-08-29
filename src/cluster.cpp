/*
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
#include "file_reader.h"
#include "endian_tools.h"
#include <algorithm>
#include <stdlib.h>
#include <sstream>

#include "compression.h"
#include "log.h"

#include "config.h"

log_define("zim.cluster")

#define log_debug1(e)

namespace zim
{

namespace
{

const Buffer
getClusterBuffer(const Reader& zimReader, offset_t offset, CompressionType comp)
{
  zsize_t uncompressed_size(0);
  std::unique_ptr<char[]> uncompressed_data;
  switch (comp) {
    case zimcompLzma:
      uncompressed_data = uncompress<LZMA_INFO>(&zimReader, offset, &uncompressed_size);
      break;
    case zimcompZip:
#if defined(ENABLE_ZLIB)
      uncompressed_data = uncompress<ZIP_INFO>(&zimReader, offset, &uncompressed_size);
#else
      throw std::runtime_error("zlib not enabled in this library");
#endif
      break;
    case zimcompZstd:
      uncompressed_data = uncompress<ZSTD_INFO>(&zimReader, offset, &uncompressed_size);
      break;
    default:
      throw std::logic_error("compressions should not be something else than zimcompLzma, zimComZip or zimcompZstd.");
  }
  auto shared_data = std::shared_ptr<const char>(uncompressed_data.release(), std::default_delete<char[]>());
  return Buffer::makeBuffer(shared_data, uncompressed_size);
}

std::unique_ptr<const Reader>
getClusterReader(const Reader& zimReader, offset_t offset, CompressionType* comp, bool* extended)
{
  uint8_t clusterInfo = zimReader.read(offset);
  *comp = static_cast<CompressionType>(clusterInfo & 0x0F);
  *extended = clusterInfo & 0x10;

  switch (*comp) {
    case zimcompDefault:
    case zimcompNone:
      {
      // No compression, just a sub_reader
        return zimReader.sub_reader(offset+offset_t(1));
      }
      break;
    case zimcompLzma:
    case zimcompZip:
    case zimcompZstd:
      {
        auto buffer = getClusterBuffer(zimReader, offset+offset_t(1), *comp);
        return std::unique_ptr<Reader>(new BufferReader(buffer));
      }
      break;
    case zimcompBzip2:
      throw std::runtime_error("bzip2 not enabled in this library");
    default:
      throw ZimFileFormatError("Invalid compression flag");
  }
}

} // unnamed namespace

  std::shared_ptr<Cluster> Cluster::read(const Reader& zimReader, offset_t clusterOffset)
  {
    CompressionType comp;
    bool extended;
    std::shared_ptr<const Reader> reader = getClusterReader(zimReader, clusterOffset, &comp, &extended);
    return std::make_shared<Cluster>(reader, comp, extended);
  }

  Cluster::Cluster(std::shared_ptr<const Reader> reader_, CompressionType comp, bool isExtended)
    : compression(comp),
      isExtended(isExtended),
      reader(reader_),
      startOffset(0)
  {
    auto d = reader->offset();
    if (isExtended) {
      startOffset = read_header<uint64_t>();
    } else {
      startOffset = read_header<uint32_t>();
    }
    reader = reader->sub_reader(startOffset, zsize_t(offsets.back().v));
    auto d1 = reader->offset();
    ASSERT(d+startOffset, ==, d1);
  }

  /* This return the number of char read */
  template<typename OFFSET_TYPE>
  offset_t Cluster::read_header()
  {
    // read first offset, which specifies, how many offsets we need to read
    OFFSET_TYPE offset = reader->read_uint<OFFSET_TYPE>(offset_t(0));

    size_t n_offset = offset / sizeof(OFFSET_TYPE);
    const offset_t data_address(offset);

    // read offsets
    offsets.clear();
    offsets.reserve(n_offset);
    offsets.push_back(offset_t(0));

    auto buffer = reader->get_buffer(offset_t(0), zsize_t(offset));
    offset_t current = offset_t(sizeof(OFFSET_TYPE));
    while (--n_offset)
    {
      OFFSET_TYPE new_offset = buffer.as<OFFSET_TYPE>(current);
      ASSERT(new_offset, >=, offset);
      ASSERT(new_offset, <=, reader->size().v);

      offset = new_offset;
      offsets.push_back(offset_t(offset - data_address.v));
      current += sizeof(OFFSET_TYPE);
    }
    return data_address;
  }

  zsize_t Cluster::getBlobSize(blob_index_t n) const
  {
      if (blob_index_type(n)+1 >= offsets.size()) throw ZimFileFormatError("blob index out of range");
      return zsize_t(offsets[blob_index_type(n)+1].v - offsets[blob_index_type(n)].v);
  }

  Blob Cluster::getBlob(blob_index_t n) const
  {
    if (n < count()) {
      auto blobSize = getBlobSize(n);
      if (blobSize.v > SIZE_MAX) {
        return Blob();
      }
      auto buffer = reader->get_buffer(offsets[blob_index_type(n)], blobSize);
      return buffer;
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
      offset += offsets[blob_index_type(n)];
      auto buffer = reader->get_buffer(offset, size);
      return buffer;
    } else {
      return Blob();
    }
  }

}
