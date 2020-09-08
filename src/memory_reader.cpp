/*
 * Copyright (C) 2017 Matthieu Gautier <mgautier@kymeria.fr>
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
#include "memory_reader.h"
#include "buffer.h"
#include <string.h>

namespace zim
{

Blob MemoryReader::read_blob(offset_t offset, zsize_t size) const
{
  return data_.subBlob(offset.v, size.v);
}

std::shared_ptr<const Buffer>
MemoryReader::get_buffer(offset_t offset, zsize_t size) const
{
  // must never reach here
  throw std::logic_error("MemoryReader::get_buffer() must not be called");
}

std::unique_ptr<const Reader>
MemoryReader::sub_reader(offset_t offset, zsize_t size) const
{
  return std::unique_ptr<Reader>(new MemoryReader(read_blob(offset, size)));
}

zsize_t MemoryReader::size() const
{
  return zsize_t(data_.size());
}

offset_t MemoryReader::offset() const
{
  // must never reach here
  throw std::logic_error("MemoryReader::offset() must not be called");
}


void MemoryReader::read(char* dest, offset_t offset, zsize_t size) const
{
  ASSERT(offset.v, <, data_.size());
  ASSERT(offset.v+size.v, <=, data_.size());
  if ( size ) {
    memcpy(dest, data_.data() + offset.v, size.v);
  }
}


char MemoryReader::read(offset_t offset) const
{
  ASSERT(offset.v, <, data_.size());
  return data_.data()[offset.v];
}


} // namespace zim
