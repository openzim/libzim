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

#include "endian_tools.h"
#include "debug.h"

namespace zim {

class Buffer;
class FileCompound;

class Reader {
  public:
    Reader() {};
    virtual offset_type size() const = 0;
    virtual ~Reader() {};

    virtual void read(char* dest, offset_type offset, offset_type size) const = 0;
    template<typename T>
    T read(offset_type offset) const {
      ASSERT(offset, <, size());
      ASSERT(offset+sizeof(T), <=, size());
      char tmp_buf[sizeof(T)];
      read(tmp_buf, offset, sizeof(T));
      return fromLittleEndian<T>(tmp_buf);
    }
    virtual char read(offset_type offset) const = 0;

    virtual std::shared_ptr<const Buffer> get_buffer(offset_type offset, offset_type size) const = 0;
    std::shared_ptr<const Buffer> get_buffer(offset_type offset) const {
      return get_buffer(offset, size()-offset);
    }
    virtual std::unique_ptr<const Reader> sub_reader(offset_type offset, offset_type size) const = 0;
    std::unique_ptr<const Reader> sub_reader(offset_type offset) const {
      return sub_reader(offset, size()-offset);
    }
    virtual offset_type offset() const = 0;

    std::unique_ptr<const Reader> sub_clusterReader(offset_type offset, offset_type size, CompressionType* comp) const;

    bool can_read(offset_type offset, offset_type size);

  private:
    std::shared_ptr<const Buffer> get_clusterBuffer(offset_type offset, offset_type size, CompressionType comp) const;
    virtual std::unique_ptr<const Reader> get_mmap_sub_reader(offset_type offset, offset_type size) const = 0;

};

class FileReader : public Reader {
  public:
    FileReader(std::shared_ptr<const FileCompound> source);
    ~FileReader() {};

    offset_type size() const { return _size; };
    offset_type offset() const { return _offset; };

    char read(offset_type offset) const;
    void read(char* dest, offset_type offset, offset_type size) const;
    std::shared_ptr<const Buffer> get_buffer(offset_type offset, offset_type size) const;

    std::unique_ptr<const Reader> sub_reader(offset_type offest, offset_type size) const;

  private:
    FileReader(std::shared_ptr<const FileCompound> source, offset_type offset);
    FileReader(std::shared_ptr<const FileCompound> source, offset_type offset, offset_type size);

    virtual std::unique_ptr<const Reader> get_mmap_sub_reader(offset_type offset, offset_type size) const;

    std::shared_ptr<const FileCompound> source;
    offset_type _offset;
    offset_type _size;
};

class BufferReader : public Reader {
  public:
    BufferReader(std::shared_ptr<const Buffer> source)
      : source(source) {}
    virtual ~BufferReader() {};

    offset_type size() const;
    offset_type offset() const;

    void read(char* dest, offset_type offset, offset_type size) const;
    char read(offset_type offset) const;
    std::shared_ptr<const Buffer> get_buffer(offset_type offset, offset_type size) const;
    std::unique_ptr<const Reader> sub_reader(offset_type offset, offset_type size) const;

  private:
    virtual std::unique_ptr<const Reader> get_mmap_sub_reader(offset_type offset, offset_type size) const
      { return std::unique_ptr<Reader>(); }
    std::shared_ptr<const Buffer> source;
};

};

#endif // ZIM_FILE_READER_H_
