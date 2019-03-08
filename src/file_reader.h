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

#include <memory>

#include "zim_types.h"
#include "endian_tools.h"
#include "debug.h"

namespace zim {

class Buffer;
class FileCompound;

class Reader {
  public:
    Reader() {};
    virtual zsize_t size() const = 0;
    virtual ~Reader() {};

    virtual void read(char* dest, offset_t offset, zsize_t size) const = 0;
    template<typename T>
    T read(offset_t offset) const {
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

    std::unique_ptr<const Reader> sub_clusterReader(offset_t offset,
                                                    CompressionType* comp,
                                                    bool* extented) const;

    bool can_read(offset_t offset, zsize_t size);

  private:
    std::shared_ptr<const Buffer> get_clusterBuffer(offset_t offset, CompressionType comp) const;
};

class FileReader : public Reader {
  public:
    FileReader(std::shared_ptr<const FileCompound> source);
    ~FileReader() {};

    zsize_t size() const { return _size; };
    offset_t offset() const { return _offset; };

    char read(offset_t offset) const;
    void read(char* dest, offset_t offset, zsize_t size) const;
    std::shared_ptr<const Buffer> get_buffer(offset_t offset, zsize_t size) const;

    std::unique_ptr<const Reader> sub_reader(offset_t offest, zsize_t size) const;

  private:
    FileReader(std::shared_ptr<const FileCompound> source, offset_t offset);
    FileReader(std::shared_ptr<const FileCompound> source, offset_t offset, zsize_t size);

    std::shared_ptr<const FileCompound> source;
    offset_t _offset;
    zsize_t _size;
};

class BufferReader : public Reader {
  public:
    BufferReader(std::shared_ptr<const Buffer> source)
      : source(source) {}
    virtual ~BufferReader() {};

    zsize_t size() const;
    offset_t offset() const;

    void read(char* dest, offset_t offset, zsize_t size) const;
    char read(offset_t offset) const;
    std::shared_ptr<const Buffer> get_buffer(offset_t offset, zsize_t size) const;
    std::unique_ptr<const Reader> sub_reader(offset_t offset, zsize_t size) const;

  private:
    std::shared_ptr<const Buffer> source;
};

};

#endif // ZIM_FILE_READER_H_
