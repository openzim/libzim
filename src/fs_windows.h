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

#ifndef ZIM_FS_WINDOWS_H_
#define ZIM_FS_WINDOWS_H_

#include "zim_types.h"

#include <stdexcept>
#include <memory>

typedef void* HANDLE;

namespace zim {

namespace windows {

using path_t = const std::string&;

struct ImplFD;

class FD {
  public:
    typedef HANDLE fd_t;
  private:
    std::unique_ptr<ImplFD> mp_impl;

  public:
    FD();
    FD(fd_t handle);
    FD(int fd);
    FD(const FD& o) = delete;
    FD(FD&& o);
    FD& operator=(FD&& o);
    FD& operator=(const FD& o) = delete;
    ~FD();
    zsize_t readAt(char* dest, zsize_t size, offset_t offset) const;
    zsize_t getSize() const;
    int     release();
    bool    seek(offset_t offset);
    bool    close();
};

struct FS {
    using FD = zim::windows::FD;
    static std::string join(path_t base, path_t name);
    static std::unique_ptr<wchar_t[]> toWideChar(path_t path);
    static FD   openFile(path_t filepath);
    static bool makeDirectory(path_t path);
    static void rename(path_t old_path, path_t new_path);
    static bool remove(path_t path);
    static bool removeDir(path_t path);
    static bool removeFile(path_t path);
};

}; // windows namespace

}; // zim namespace

#endif //ZIM_FS_WINDOWS_H_
