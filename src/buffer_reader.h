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

#ifndef ZIM_BUFFER_READER_H_
#define ZIM_BUFFER_READER_H_

#include "reader.h"

namespace zim {

class BufferReader : public Reader {
  public:
    BufferReader(const Buffer& source)
      : source(source) {}
    virtual ~BufferReader() {};

    zsize_t size() const;
    offset_t offset() const;

    void read(char* dest, offset_t offset, zsize_t size) const;
    char read(offset_t offset) const;
    const Buffer get_buffer(offset_t offset, zsize_t size) const;
    std::unique_ptr<const Reader> sub_reader(offset_t offset, zsize_t size) const;

  private:
    const Buffer source;
};

};

#endif // ZIM_BUFFER_READER_H_
