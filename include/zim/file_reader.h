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
#include <cassert>

namespace zim {

class Buffer;
class FileCompound;

class Reader {
  public:
    Reader() {};
    virtual std::size_t size() const = 0;
    virtual ~Reader() {};

    virtual void read(char* dest, std::size_t offset, std::size_t size) = 0;
    template<typename T>
    T read(std::size_t offset) {
      assert(offset < size());
      assert(offset+sizeof(T) <= size());
      T ret;
      read(reinterpret_cast<char*>(&ret), offset, sizeof(T));
      return ret;
    }
    virtual char read(std::size_t offset) = 0;

    virtual std::shared_ptr<Buffer> get_buffer(std::size_t offset, std::size_t size) = 0;
    std::shared_ptr<Buffer> get_buffer(std::size_t offset) {
      return get_buffer(offset, size()-offset);
    }
    virtual std::unique_ptr<Reader> sub_reader(std::size_t offset, std::size_t size) = 0;
    std::unique_ptr<Reader> sub_reader(std::size_t offset) {
      return sub_reader(offset, size()-offset);
    }
    virtual std::size_t offset() const = 0;

    std::unique_ptr<Reader> sub_clusterReader(std::size_t offset, std::size_t size, CompressionType* comp);

  private:
    std::shared_ptr<Buffer> get_clusterBuffer(std::size_t offset, std::size_t size, CompressionType comp);
    virtual std::unique_ptr<Reader> get_mmap_sub_reader(std::size_t offset, std::size_t size) = 0;

};

class FileReader : public Reader {
  public:
    FileReader(std::shared_ptr<FileCompound> source);
    ~FileReader() {};

    std::size_t size() const { return _size; };
    std::size_t offset() const { return _offset; };

    char read(std::size_t offset);
    void read(char* dest, std::size_t offset, std::size_t size);
    std::shared_ptr<Buffer> get_buffer(std::size_t offset, std::size_t size);

    std::unique_ptr<Reader> sub_reader(std::size_t offest, std::size_t size);

  private:
    FileReader(std::shared_ptr<FileCompound> source, std::size_t offset);
    FileReader(std::shared_ptr<FileCompound> source, std::size_t offset, std::size_t size);

     virtual std::unique_ptr<Reader> get_mmap_sub_reader(std::size_t offset, std::size_t size);

    std::shared_ptr<FileCompound> source;
    std::size_t _offset;
    std::size_t _size;
};

class BufferReader : public Reader {
  public:
    BufferReader(std::shared_ptr<Buffer> source)
      : source(source) {}
    virtual ~BufferReader() {};

    std::size_t size() const;
    std::size_t offset() const;

    void read(char* dest, std::size_t offset, std::size_t size);
    char read(std::size_t offset);
    std::shared_ptr<Buffer> get_buffer(std::size_t offset, std::size_t size);
    std::unique_ptr<Reader> sub_reader(std::size_t offset, std::size_t size);

  private:
     virtual std::unique_ptr<Reader> get_mmap_sub_reader(std::size_t offset, std::size_t size) { return std::unique_ptr<Reader>(); }
    std::shared_ptr<Buffer> source;
};

};

#endif // ZIM_FILE_READER_H_
