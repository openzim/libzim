/*
 * Copyright (C) 2018 Matthieu Gautier <mgautier@kymeria.fr>
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

#ifndef ZIM_FS_UNIX_H_
#define ZIM_FS_UNIX_H_

#include "zim_types.h"

#include <stdexcept>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

namespace zim {

namespace unix {

using path_t = const std::string&;

class FD {
  public:
    using fd_t = int;

  private:
    fd_t m_fd = -1;

  public:
    FD() = default;
    FD(fd_t fd):
      m_fd(fd) {};
    FD(const FD& o) = delete;
    FD(FD&& o) :
      m_fd(o.m_fd) { o.m_fd = -1; }
    FD& operator=(FD&& o) {
      m_fd = o.m_fd;
      o.m_fd = -1;
      return *this;
    }
    ~FD() { close(); }
    zsize_t readAt(char* dest, zsize_t size, offset_t offset) const;
    zsize_t getSize() const;
    fd_t    getNativeHandle() const
    {
        return m_fd;
    }
    fd_t    release()
    {
        int ret = m_fd;
        m_fd = -1;
        return ret;
    }
    bool    seek(offset_t offset);
    bool    close();
};

struct FS {
    using FD = zim::unix::FD;
    static std::string join(path_t base, path_t name);
    static FD    openFile(path_t filepath);
    static bool  makeDirectory(path_t path);
    static void  rename(path_t old_path, path_t new_path);
    static bool  remove(path_t path);
    static bool  removeDir(path_t path);
    static bool  removeFile(path_t path);
};

}; // unix namespace

std::string getFilePathFromFD(int fd);

}; // zim namespace

#endif //ZIM_FS_UNIX_H_
