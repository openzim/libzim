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

#include "log.h"

#include "config.h"

log_define("zim.cluster")

#define log_debug1(e)

namespace zim
{
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
    reader = reader->sub_reader(startOffset);
    auto d1 = reader->offset();
    ASSERT(d+startOffset, ==, d1);
  }

  /* This return the number of char read */
  template<typename OFFSET_TYPE>
  offset_t Cluster::read_header()
  {
    // read first offset, which specifies, how many offsets we need to read
    OFFSET_TYPE offset;
    offset = reader->read<OFFSET_TYPE>(offset_t(0));

    size_t n_offset = offset / sizeof(OFFSET_TYPE);
    offset_t data_address(offset);

    // read offsets
    offsets.clear();
    offsets.reserve(n_offset);
    offsets.push_back(offset_t(0));
    
    auto buffer = reader->get_buffer(offset_t(0), zsize_t(offset));
    offset_t current = offset_t(sizeof(OFFSET_TYPE));
    while (--n_offset)
    {
      OFFSET_TYPE new_offset = buffer->as<OFFSET_TYPE>(current);
      ASSERT(new_offset, >=, offset);
      ASSERT(offset, >=, data_address.v);
      ASSERT(offset, <=, reader->size().v);
      
      offset = new_offset;
      offsets.push_back(offset_t(offset - data_address.v));
      current += sizeof(OFFSET_TYPE);
    }
    ASSERT(offset, ==, reader->size().v);
    return data_address;
  }

  Blob Cluster::getBlob(blob_index_t n) const
  {
    if (size()) {
      auto blobSize = getBlobSize(n);
      if (blobSize.v > SIZE_MAX) {
        return Blob();
      }
      auto buffer = reader->get_buffer(offsets[blob_index_type(n)], getBlobSize(n));
      return Blob(buffer);
    } else {
      return Blob();
    }
  }

  Blob Cluster::getBlob(blob_index_t n, offset_t offset, zsize_t size) const
  {
    if (this->size()) {
      offset += offsets[blob_index_type(n)];
      size = std::min(size, getBlobSize(n));
      if (size.v > SIZE_MAX) {
        return Blob();
      }
      auto buffer = reader->get_buffer(offset, size);
      return Blob(buffer);
    } else {
      return Blob();
    }
  }

  zsize_t Cluster::size() const
  {
    if (isExtended)
      return zsize_t(offsets.size() * sizeof(uint64_t) + reader->size().v);
    else
      return zsize_t(offsets.size() * sizeof(uint32_t) + reader->size().v);
  }

  template<typename OFFSET_TYPE>
  zsize_t _read_size(const Reader* reader, offset_t offset)
  {
    OFFSET_TYPE blob_offset = reader->read<OFFSET_TYPE>(offset);
    auto off = offset+offset_t(blob_offset-sizeof(OFFSET_TYPE));
    auto s = reader->read<OFFSET_TYPE>(off);
    return zsize_t(s);
  }

  zsize_t Cluster::read_size(const Reader* reader, bool isExtended, offset_t offset)
  {
    if (isExtended)
      return _read_size<uint64_t>(reader, offset);
    else
      return _read_size<uint32_t>(reader, offset);
  }

}
