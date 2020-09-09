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

namespace {

class SubBuffer : public Buffer {
  public:
    SubBuffer(const std::shared_ptr<const Buffer> src, offset_t offset, zsize_t size)
      : Buffer(size),
        _data(src, src->data(offset))
    {
      ASSERT(offset.v, <=, src->size().v);
      ASSERT(offset.v+size.v, <=, src->size().v);
    }

  const char* dataImpl(offset_t offset) const {
        return _data.get() + offset.v;
    }

  private:
    const std::shared_ptr<const char> _data;
};

} // unnamed namespace

std::shared_ptr<const Buffer> Buffer::sub_buffer(offset_t offset, zsize_t size) const
{
  return std::make_shared<SubBuffer>(shared_from_this(), offset, size);
}

} //zim
