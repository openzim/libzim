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

#ifndef ZIM_BUFFER_H_
#define ZIM_BUFFER_H_

#include <cstddef>
#include <exception>
#include <memory>
#include <iostream>

#include "config.h"
#include "zim_types.h"
#include "endian_tools.h"
#include "debug.h"

namespace zim {

class MMapException : std::exception {};

class Buffer : public std::enable_shared_from_this<Buffer> {
  public:
    Buffer(zsize_t size)
      : size_(size)
    {
      ASSERT(size_.v, <, SIZE_MAX);
    };
    virtual ~Buffer() {};
    virtual const char* data(offset_t offset=offset_t(0)) const = 0;
    virtual char at(offset_t offset) const {
        return *(data(offset));
    }
    zsize_t size() const { return size_; }
    virtual std::shared_ptr<const Buffer> sub_buffer(offset_t offset, zsize_t size) const;

    template<typename T>
    T as(offset_t offset) const {
      ASSERT(offset.v, <, size_.v);
      ASSERT(offset.v+sizeof(T), <=, size_.v);
      return fromLittleEndian<T>(data(offset));
    }

  protected:
    const zsize_t size_;
};


template<bool CLEAN_AT_END>
class MemoryBuffer : public Buffer {
  public:
    MemoryBuffer(const char* buffer, zsize_t size)
      : Buffer(size),
        _data(buffer)
    {}

    virtual ~MemoryBuffer() {
        if ( CLEAN_AT_END ) {
          delete [] _data;
        }
    }

    const char* data(offset_t offset) const {
        ASSERT(offset.v, <=, size_.v);
        return _data + offset.v;
    }
  private:
    const char* _data;
};


#ifdef ENABLE_USE_MMAP
class MMapBuffer : public Buffer {
  public:
    MMapBuffer(int fd, offset_t offset, zsize_t size);
    ~MMapBuffer();

    const char* data(offset_t offset) const {
      offset += _offset;
      return _data + offset.v;
    }

  private:
    offset_t _offset;
    char* _data;
};
#endif


class SubBuffer : public Buffer {
  public:
    SubBuffer(const std::shared_ptr<const Buffer> src, offset_t offset, zsize_t size)
      : Buffer(size),
        _data(src, src->data(offset))
    {
      ASSERT(offset.v+size.v, <=, src->size().v);
    }

  const char* data(offset_t offset) const {
        ASSERT(offset.v, <=, size_.v);
        return _data.get() + offset.v;
    }

  private:
    std::shared_ptr<const char> _data;
};

};

#endif //ZIM_BUFFER_H_
