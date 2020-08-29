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

#ifndef ZIM_READER_H_
#define ZIM_READER_H_

#include <memory>

#include "zim_types.h"
#include "endian_tools.h"
#include "debug.h"

namespace zim {

class Buffer;

class Reader {
  public:
    Reader() {};
    virtual zsize_t size() const = 0;
    virtual ~Reader() {};

    virtual void read(char* dest, offset_t offset, zsize_t size) const = 0;
    template<typename T>
    T read_uint(offset_t offset) const {
      ASSERT(offset.v, <, size().v);
      ASSERT(offset.v+sizeof(T), <=, size().v);
      char tmp_buf[sizeof(T)];
      read(tmp_buf, offset, zsize_t(sizeof(T)));
      return fromLittleEndian<T>(tmp_buf);
    }
    virtual char read(offset_t offset) const = 0;

    virtual std::shared_ptr<const Buffer> get_buffer(offset_t offset, zsize_t size) const = 0;
    std::shared_ptr<const Buffer> get_buffer(offset_t offset) const {
      return get_buffer(offset, zsize_t(size().v-offset.v));
    }
    virtual std::unique_ptr<const Reader> sub_reader(offset_t offset, zsize_t size) const = 0;
    std::unique_ptr<const Reader> sub_reader(offset_t offset) const {
      return sub_reader(offset, zsize_t(size().v-offset.v));
    }
    virtual offset_t offset() const = 0;

    bool can_read(offset_t offset, zsize_t size);
};

};

#endif // ZIM_READER_H_
