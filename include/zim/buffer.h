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
#include <cassert>
#include <iostream>

namespace zim {

class BufferError : std::exception {};

class Buffer : public std::enable_shared_from_this<Buffer> {
  public:
    Buffer(std::size_t size)
      : size_(size) {};
    virtual ~Buffer() {};
    virtual const char* data(std::size_t offset=0) const = 0;
    virtual char at(std::size_t offset) {
        return *(data(offset));
    }
    std::size_t size() const { return size_; }
    virtual std::shared_ptr<Buffer> sub_buffer(std::size_t offset, std::size_t size);

    template<typename T>
    const T* as(size_t offset) const {
      assert(offset < size_);
      assert(offset+sizeof(T) <= size_);
      return reinterpret_cast<const T*>(data(offset));
    }

  protected:
    std::size_t size_;
};


template<bool CLEAN_AT_END>
class MemoryBuffer : public Buffer {
  public:
    MemoryBuffer(const char* buffer, std::size_t size)
      : Buffer(size),
        _data(buffer)
    {}

    virtual ~MemoryBuffer() {
        if ( CLEAN_AT_END ) {
          delete [] _data;
        }
    }

    const char* data(std::size_t offset) const {
        assert(offset <= size_);
        return _data + offset;
    }
  private:
    const char* _data;
};


#if !defined(_WIN32)
class MMapBuffer : public Buffer {
  public:
    MMapBuffer(const std::string& filename, std::size_t offset, std::size_t size);
    ~MMapBuffer();

    const char* data(std::size_t offset) const {
      offset += _offset;
      return _data + offset;
    }

  private:
    int fd;
    std::size_t _offset;
    char* _data;
};
#endif


class SubBuffer : public Buffer {
  public:
    SubBuffer(const std::shared_ptr<Buffer> src, std::size_t offset, std::size_t size)
      : Buffer(size),
        _data(src, src->data(offset))
    {
      assert((offset+size <= src->size()));
    }

  const char* data(std::size_t offset) const {
        assert(offset <= size_);
        return _data.get() + offset;
    }    

  private:
    std::shared_ptr<const char> _data;
};

};

#endif //ZIM_BUFFER_H_
