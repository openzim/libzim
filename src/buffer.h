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
  public: // types
    typedef std::shared_ptr<const char> DataPtr;

  public: // functions
    explicit Buffer(const char* data, zsize_t size);
    explicit Buffer(const DataPtr& data, zsize_t size);
    explicit Buffer(zsize_t size);

    Buffer(const Buffer& ) = delete;
    void operator=(const Buffer& ) = delete;

    const char* data(offset_t offset=offset_t(0)) const;

    char at(offset_t offset) const {
      return *(data(offset));
    }
    zsize_t size() const { return m_size; }
    std::shared_ptr<const Buffer> sub_buffer(offset_t offset, zsize_t size) const;

    template<typename T>
    T as(offset_t offset) const {
      ASSERT(offset.v, <, m_size.v);
      ASSERT(offset.v+sizeof(T), <=, m_size.v);
      return fromLittleEndian<T>(data(offset));
    }

  private: //data
    const zsize_t m_size;
    DataPtr m_data;
};

} // zim namespace

#endif //ZIM_BUFFER_H_
