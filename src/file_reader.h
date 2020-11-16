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

#ifndef ZIM_FILE_READER_H_
#define ZIM_FILE_READER_H_

#include "reader.h"

namespace zim {

class FileCompound;

class MultiPartFileReader : public Reader {
  public:
    MultiPartFileReader(std::shared_ptr<const FileCompound> source);
    ~MultiPartFileReader() {};

    zsize_t size() const { return _size; };
    offset_t offset() const { return _offset; };

    char read(offset_t offset) const;
    void read(char* dest, offset_t offset, zsize_t size) const;
    const Buffer get_buffer(offset_t offset, zsize_t size) const;

    std::unique_ptr<const Reader> sub_reader(offset_t offest, zsize_t size) const;

  private:
    MultiPartFileReader(std::shared_ptr<const FileCompound> source, offset_t offset);
    MultiPartFileReader(std::shared_ptr<const FileCompound> source, offset_t offset, zsize_t size);

    std::shared_ptr<const FileCompound> source;
    offset_t _offset;
    zsize_t _size;
};

};

#endif // ZIM_FILE_READER_H_
