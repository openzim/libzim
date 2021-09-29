/*
 * Copyright (C) 2020 Veloman Yunkan
 * Copyright (C) 2017-2020 Matthieu Gautier <mgautier@kymeria.fr>
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

#include "buffer.h"

#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sstream>

#ifndef _WIN32
#  include <sys/mman.h>
#  include <unistd.h>
#endif

namespace zim {

namespace
{

struct NoDelete
{
  template<class T> void operator()(T*) {}
};

// This shared_ptr is used as a source object for the std::shared_ptr
// aliasing constructor (with the purpose of avoiding the control block
// allocation) for the case when the referred data must not be deleted.
static Buffer::DataPtr nonOwnedDataPtr((char*)nullptr, NoDelete());

} // unnamed namespace

const Buffer Buffer::sub_buffer(offset_t offset, zsize_t size) const
{
  ASSERT(offset.v, <=, m_size.v);
  ASSERT(offset.v+size.v, <=, m_size.v);
  auto sub_data = DataPtr(m_data, data(offset));
  return Buffer(sub_data, size);
}

const Buffer Buffer::makeBuffer(const DataPtr& data, zsize_t size)
{
  return Buffer(data, size);
}

const Buffer Buffer::makeBuffer(const char* data, zsize_t size)
{
  return Buffer(DataPtr(nonOwnedDataPtr, data), size);
}

Buffer Buffer::makeBuffer(zsize_t size)
{
  if (0 == size.v) {
    return Buffer(DataPtr(nonOwnedDataPtr, nullptr), size);
  }
  return Buffer(DataPtr(new char[size.v], std::default_delete<char[]>()), size);
}

Buffer::Buffer(const DataPtr& data, zsize_t size)
  : m_size(size),
    m_data(data)
{
  ASSERT(m_size.v, <, SIZE_MAX);
}

const char*
Buffer::data(offset_t offset) const {
  ASSERT(offset.v, <=, m_size.v);
  return m_data.get() + offset.v;
}

} //zim
