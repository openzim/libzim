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

} // unnamed namespace

std::shared_ptr<const Buffer> Buffer::sub_buffer(offset_t offset, zsize_t size) const
{
  ASSERT(offset.v, <=, m_size.v);
  ASSERT(offset.v+size.v, <=, m_size.v);
  auto sub_data = DataPtr(m_data, data(offset));
  return std::make_shared<Buffer>(sub_data, size);
}

Buffer::Buffer(const DataPtr& data, zsize_t size)
  : m_size(size),
    m_data(data)
{
  ASSERT(m_size.v, <, SIZE_MAX);
}

Buffer::Buffer(const char* data, zsize_t size)
  : Buffer(DataPtr(data, NoDelete()), size)
{}

Buffer::Buffer(zsize_t size)
  : Buffer(DataPtr(new char[size.v], std::default_delete<char[]>()), size)
{}

const char*
Buffer::data(offset_t offset) const {
  ASSERT(offset.v, <=, m_size.v);
  return m_data.get() + offset.v;
}

} //zim
