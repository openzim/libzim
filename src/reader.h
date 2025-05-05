/*
 * Copyright (C) 2017-2020 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2020 Veloman Yunkan
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
#include <stdexcept>

#include "zim_types.h"
#include "endian_tools.h"
#include "debug.h"

#include "buffer.h"

namespace zim {

class LIBZIM_PRIVATE_API Reader {
  public:
    Reader() {};

    // Returns the full size of data accessible via this reader object
    virtual zsize_t size() const = 0;

    // Returns the memory consumption by this reader object
    virtual size_t getMemorySize() const = 0;

    virtual ~Reader() {};

    void read(char* dest, offset_t offset, zsize_t size) const {
      if (can_read(offset, size)) {
        if (size) {
          // Do the actuall read only if we have a size to read
          readImpl(dest, offset, size);
        }
        return;
      }
      throw std::runtime_error("Cannot read after the end of the reader");
    }

    template<typename T>
    T read_uint(offset_t offset) const {
      ASSERT(offset.v, <, size().v);
      ASSERT(offset.v+sizeof(T), <=, size().v);
      char tmp_buf[sizeof(T)];
      read(tmp_buf, offset, zsize_t(sizeof(T)));
      return fromLittleEndian<T>(tmp_buf);
    }

    char read(offset_t offset) const {
      if (can_read(offset, zsize_t(1))) {
        return readImpl(offset);
      }
      throw std::runtime_error("Cannot read after the end of the reader");
    }

    virtual const Buffer get_buffer(offset_t offset, zsize_t size) const = 0;
    const Buffer get_buffer(offset_t offset) const {
      return get_buffer(offset, zsize_t(size().v-offset.v));
    }
    virtual std::unique_ptr<const Reader> sub_reader(offset_t offset, zsize_t size) const = 0;
    std::unique_ptr<const Reader> sub_reader(offset_t offset) const {
      return sub_reader(offset, zsize_t(size().v-offset.v));
    }
    virtual offset_t offset() const = 0;

    bool can_read(offset_t offset, zsize_t size) const;

  private:
    // Implementation of the read method.
    // Check of the validity of the offset/size has already been done.
    virtual void readImpl(char* dest, offset_t offset, zsize_t size) const = 0;

    // Implementation of the read method.
    // Check of the validity of the offset has already been done.
    virtual char readImpl(offset_t offset) const = 0;
};

};

#endif // ZIM_READER_H_
