/*
 * Copyright (C) 2017-2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#ifndef ZIM_FILE_READER_H_
#define ZIM_FILE_READER_H_

#include "reader.h"
#include "fs.h"

namespace zim {

class FileCompound;

class LIBZIM_PRIVATE_API BaseFileReader : public Reader {
  public: // functions
    BaseFileReader(offset_t offset, zsize_t size)
      : _offset(offset), _size(size) {}
    ~BaseFileReader() = default;
    zsize_t size() const { return _size; };
    offset_t offset() const { return _offset; };

    virtual const Buffer get_mmap_buffer(offset_t offset,
                                         zsize_t size) const = 0;
    const Buffer get_buffer(offset_t offset, zsize_t size) const;

  protected: // data
    offset_t _offset;
    zsize_t _size;
};

class LIBZIM_PRIVATE_API FileReader : public BaseFileReader {
  public: // types
    typedef std::shared_ptr<const DEFAULTFS::FD> FileHandle;

  public: // functions
    FileReader(FileHandle fh, offset_t offset, zsize_t size);
    ~FileReader() = default;

    const Buffer get_mmap_buffer(offset_t offset, zsize_t size) const;
    std::unique_ptr<const Reader> sub_reader(offset_t offset, zsize_t size) const;

  private: // functions
    char readImpl(offset_t offset) const override;
    void readImpl(char *dest, offset_t offset, zsize_t size) const override;

  private: // data
    // The file handle is stored via a shared pointer so that it can be shared
    // by a sub_reader (otherwise the file handle would be invalidated by
    // FD destructor when the sub-reader is destroyed).
    FileHandle _fhandle;
};

class LIBZIM_PRIVATE_API MultiPartFileReader : public BaseFileReader {
  public:
    explicit MultiPartFileReader(std::shared_ptr<const FileCompound> source);
    ~MultiPartFileReader() {};

    const Buffer get_mmap_buffer(offset_t offset, zsize_t size) const;
    std::unique_ptr<const Reader> sub_reader(offset_t offset, zsize_t size) const;

  private: // functions
    char readImpl(offset_t offset) const override;
    void readImpl(char *dest, offset_t offset, zsize_t size) const override;

  private: // data
    MultiPartFileReader(std::shared_ptr<const FileCompound> source, offset_t offset, zsize_t size);

    std::shared_ptr<const FileCompound> source;
};

};

#endif // ZIM_FILE_READER_H_
