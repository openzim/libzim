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

#ifndef ZIM_FILE_PART_H_
#define ZIM_FILE_PART_H_

#include <string>
#include <cstdio>
#include <memory>

#include <zim/zim.h>

#include "zim_types.h"
#include "fs.h"

namespace zim {

class FilePart {
  typedef DEFAULTFS FS;

  public:
    using FDSharedPtr = std::shared_ptr<FS::FD>;

  public:
    FilePart(const std::string& filename) :
        m_filename(filename),
        m_fhandle(std::make_shared<FS::FD>(FS::openFile(filename))),
        m_size(m_fhandle->getSize()) {}
    FilePart(int fd) :
        m_filename(""),
        m_fhandle(std::make_shared<typename FS::FD>(fd)),
        m_size(m_fhandle->getSize()) {}
    ~FilePart() = default;
    const std::string& filename() const { return m_filename; };
    const FS::FD& fhandle() const { return *m_fhandle; };
    const FDSharedPtr& shareable_fhandle() const { return m_fhandle; };

    zsize_t size() const { return m_size; };
    bool fail() const { return !m_size; };
    bool good() const { return bool(m_size); };

  private:
    const std::string m_filename;
    FDSharedPtr m_fhandle;
    zsize_t m_size;
};

};

#endif //ZIM_FILE_PART_H_
