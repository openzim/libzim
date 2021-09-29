/*
 * Copyright (C) 2017-2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#include <zim/zim.h>
#include <zim/error.h>
#include "buffer_reader.h"
#include "buffer.h"

#include <cstring>

namespace zim {

const Buffer BufferReader::get_buffer(offset_t offset, zsize_t size) const
{
  return source.sub_buffer(offset, size);
}

std::unique_ptr<const Reader> BufferReader::sub_reader(offset_t offset, zsize_t size) const
{
  auto sub_buff = get_buffer(offset, size);
  std::unique_ptr<const Reader> sub_read(new BufferReader(sub_buff));
  return sub_read;
}

zsize_t BufferReader::size() const
{
  return source.size();
}

offset_t BufferReader::offset() const
{
  return offset_t((offset_type)(static_cast<const void*>(source.data(offset_t(0)))));
}


void BufferReader::read(char* dest, offset_t offset, zsize_t size) const {
  ASSERT(offset.v, <=, source.size().v);
  ASSERT(offset+offset_t(size.v), <=, offset_t(source.size().v));
  if (! size ) {
    return;
  }
  memcpy(dest, source.data(offset), size.v);
}


char BufferReader::read(offset_t offset) const {
  ASSERT(offset.v, <, source.size().v);
  char dest;
  dest = *source.data(offset);
  return dest;
}


} // zim
