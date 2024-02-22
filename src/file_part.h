/*
 * Copyright (C) 2020-2021 Veloman Yunkan
 * Copyright (C) 2017-2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#ifndef ZIM_FILE_PART_H_
#define ZIM_FILE_PART_H_

#include <string>
#include <cstdio>
#include <memory>

#include <zim/zim.h>

#include "zim_types.h"
#include "fs.h"

namespace zim {

/** A part of file.
 *
 * `FilePart` references a part(section) of a physical file.
 * Most of the time, `FilePart` will reference the whole file (m_offset==0 and m_size==m_fhandle->getSize())
 * but in some situation, it can reference only a part of the file (in case the content is store in a zip archive.)
 */
class FilePart {
  typedef DEFAULTFS FS;

  public:
    using FDSharedPtr = std::shared_ptr<FS::FD>;

  public:
    explicit FilePart(const std::string& filename) :
        m_filename(filename),
        m_fhandle(std::make_shared<FS::FD>(FS::openFile(filename))),
        m_offset(0),
        m_size(m_fhandle->getSize()) {}

#ifndef _WIN32
    explicit FilePart(int fd) :
        FilePart(getFilePathFromFD(fd)) {}

    FilePart(int fd, offset_t offset, zsize_t size):
        m_filename(getFilePathFromFD(fd)),
        m_fhandle(std::make_shared<FS::FD>(FS::openFile(m_filename))),
        m_offset(offset),
        m_size(size) {}
#endif

    ~FilePart() = default;
    const std::string& filename() const { return m_filename; };
    const FS::FD& fhandle() const { return *m_fhandle; };
    const FDSharedPtr& shareable_fhandle() const { return m_fhandle; };

    zsize_t size() const { return m_size; };
    offset_t offset() const { return m_offset; }
    bool fail() const { return !m_size; };
    bool good() const { return bool(m_size); };

  private:
    const std::string m_filename;
    FDSharedPtr m_fhandle;
    offset_t m_offset;
    zsize_t m_size;
};

};

#endif //ZIM_FILE_PART_H_
