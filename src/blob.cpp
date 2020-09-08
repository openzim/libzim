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


#include "zim/blob.h"
#include "debug.h"
#include "buffer.h"

namespace zim {

namespace
{

struct NoDelete
{
  template<class T> void operator()(T*) {}
};

} // unnamed namespace

Blob::Blob()
 : _data(0),
   _size(0)
{}

Blob::Blob(const char* data, size_type size)
 : _data(data, NoDelete()),
   _size(size)
{
  ASSERT(size, <, SIZE_MAX);
  ASSERT(data, <, (void*)(SIZE_MAX-size));
}

Blob::Blob(std::shared_ptr<const Buffer> buffer)
 : _data(buffer, buffer->data()),
   _size(size_type(buffer->size()))
{}

Blob
Blob::subBlob(size_type offset, size_type size) const
{
  ASSERT(offset, <=, _size);
  ASSERT(offset+size, <=, _size);
  return Blob(DataPtr(_data, _data.get() + offset), size);
}

std::shared_ptr<char>
Blob::makeBuffer(size_t size)
{
  return std::shared_ptr<char>(new char[size], std::default_delete<char[]>());
}

} //zim
