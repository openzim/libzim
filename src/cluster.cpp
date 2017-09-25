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

#include <zim/cluster.h>
#include <zim/blob.h>
#include "endian_tools.h"
#include <zim/error.h>
#include <zim/file_reader.h>
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
    assert((d+startOffset)==d1);
  }

  /* This return the number of char read */
  offset_type Cluster::read_header()
  {
    // read first offset, which specifies, how many offsets we need to read
    size_type offset;
    offset = reader->read<size_type>(0);
    offset = fromLittleEndian(&offset);

    size_type n_offset = offset / 4;
    size_type data_address = offset;

    // read offsets
    offsets.clear();
    offsets.reserve(n_offset);
    offsets.push_back(0);
    
    auto buffer = reader->get_buffer(0, offset);
    offset_type current = 4;
    while (--n_offset)
    {
      size_type new_offset = fromLittleEndian(buffer->as<size_type>(current));
      assert(new_offset >= offset);
      assert(offset >= data_address);
      assert(offset <= reader->size());
      
      offset = new_offset;
      offsets.push_back(offset - data_address);
      current += sizeof(size_type);
    }
    assert(offset==reader->size());
    return data_address;
  }

  Blob Cluster::getBlob(size_type n) const
  {
    if (size()) {
      auto buffer = reader->get_buffer(offsets[n], getBlobSize(n));
      return Blob(buffer);
    } else {
      return Blob();
    }
  }

  const char* Cluster::getBlobPtr(unsigned n) const
  {
     auto d = reader->get_buffer(offsets[n], getBlobSize(n))->data();
     return d;
  }

  size_type Cluster::size() const
  {
    return offsets.size() * sizeof(size_type) + reader->size();
  }

}
