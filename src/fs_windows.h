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

#include <windows.h>
#include <winbase.h>
#include <synchapi.h>
#include <fileapi.h>

namespace zim {

namespace windows {

using path_t = const std::string&;

class FD {
  public:
    typedef HANDLE fd_t;
  private:
    mutable CRITICAL_SECTION m_criticalSection;
    fd_t m_handle = INVALID_HANDLE_VALUE;

  public:
    FD() = default;
    FD(fd_t handle):
      m_handle(handle) {
      InitializeCriticalSection(&m_criticalSection);
    };
    FD(int fd);
    FD(const FD& o) = delete;
    FD(FD&& o) :
      m_handle(o.m_handle) {
      InitializeCriticalSection(&m_criticalSection);
      o.m_handle = INVALID_HANDLE_VALUE;
    }
    FD& operator=(FD&& o) {
      m_handle = o.m_handle;
      InitializeCriticalSection(&m_criticalSection);
      o.m_handle = INVALID_HANDLE_VALUE;
      return *this;
    }
    ~FD() { close(); DeleteCriticalSection(&m_criticalSection); }
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
