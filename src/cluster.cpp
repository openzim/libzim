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
  Cluster::Cluster(std::shared_ptr<const Reader> reader_, CompressionType comp)
    : compression(comp),
      reader(reader_),
      startOffset(0)
  {
    auto d = reader->offset();
    startOffset = read_header();
    reader = reader->sub_reader(startOffset);
    auto d1 = reader->offset();
    ASSERT(d+startOffset, ==, d1);
  }

  /* This return the number of char read */
  offset_t Cluster::read_header()
  {
    // read first offset, which specifies, how many offsets we need to read
    uint32_t offset;
    offset = reader->read<uint32_t>(offset_t(0));

    size_t n_offset = offset / sizeof(offset);
    offset_t data_address(offset);

    // read offsets
    offsets.clear();
    offsets.reserve(n_offset);
    offsets.push_back(offset_t(0));
    
    auto buffer = reader->get_buffer(offset_t(0), zsize_t(offset));
    offset_t current = offset_t(sizeof(uint32_t));
    while (--n_offset)
    {
      uint32_t new_offset = buffer->as<uint32_t>(current);
      ASSERT(new_offset, >=, offset);
      ASSERT(offset, >=, data_address.v);
      ASSERT(offset, <=, reader->size().v);
      
      offset = new_offset;
      offsets.push_back(offset_t(offset - data_address.v));
      current += sizeof(uint32_t);
    }
    ASSERT(offset, ==, reader->size().v);
    return data_address;
  }

  Blob Cluster::getBlob(blob_index_t n) const
  {
    if (size()) {
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
      auto buffer = reader->get_buffer(offset, size);
      return Blob(buffer);
    } else {
      return Blob();
    }
  }

  const char* Cluster::getBlobPtr(blob_index_t n) const
  {
     auto d = reader->get_buffer(offsets[blob_index_type(n)], getBlobSize(n))->data();
     return d;
  }

  zsize_t Cluster::size() const
  {
    return zsize_t(offsets.size() * sizeof(uint32_t) + reader->size().v);
  }

}
