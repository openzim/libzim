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

class Buffer : public std::enable_shared_from_this<Buffer> {
  public:
    explicit Buffer(zsize_t size)
      : size_(size)
    {
      ASSERT(size_.v, <, SIZE_MAX);
    };

    Buffer(const Buffer& ) = delete;
    void operator=(const Buffer& ) = delete;

    virtual ~Buffer() {};
    const char* data(offset_t offset=offset_t(0)) const {
      ASSERT(offset.v, <=, size_.v);
      return dataImpl(offset);
    }

    char at(offset_t offset) const {
        return *(data(offset));
    }
    zsize_t size() const { return size_; }
    std::shared_ptr<const Buffer> sub_buffer(offset_t offset, zsize_t size) const;

    template<typename T>
    T as(offset_t offset) const {
      ASSERT(offset.v, <, size_.v);
      ASSERT(offset.v+sizeof(T), <=, size_.v);
      return fromLittleEndian<T>(data(offset));
    }

  protected:
    virtual const char* dataImpl(offset_t offset) const = 0;

  protected:
    const zsize_t size_;
};


class SharedBuffer : public Buffer {
  public: // types
    typedef std::shared_ptr<const char> DataPtr;

  public: // functions
    SharedBuffer(const char* data, zsize_t size);
    SharedBuffer(const DataPtr& data, zsize_t size);
    SharedBuffer(zsize_t size);

  protected:
    const char* dataImpl(offset_t offset) const;

  private: //data
    DataPtr m_data;
};


#ifdef ENABLE_USE_MMAP
class MMapException : std::exception {};

class MMapBuffer : public Buffer {
  public:
    MMapBuffer(int fd, offset_t offset, zsize_t size);
    ~MMapBuffer();

    const char* dataImpl(offset_t offset) const;

  private:
    offset_t _offset;
    char* _data;
};
#endif

};

#endif //ZIM_BUFFER_H_
