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

#include "fs_windows.h"
#include <stdexcept>

#include <windows.h>
#include <winbase.h>
#include <synchapi.h>
#include <io.h>
#include <fileapi.h>

#include <iostream>
#include <sstream>

namespace zim {

namespace windows {

struct ImplFD {
  HANDLE m_handle = INVALID_HANDLE_VALUE;
  CRITICAL_SECTION m_criticalSection;

  ImplFD() {
    InitializeCriticalSection(&m_criticalSection);
  }
  ImplFD(HANDLE handle) :
    m_handle(handle)
  {
    InitializeCriticalSection(&m_criticalSection);
  }

  ~ImplFD() {
    DeleteCriticalSection(&m_criticalSection);
  }
};

FD::FD() :
  mp_impl(new ImplFD()) {}

FD::FD(fd_t handle) :
  mp_impl(new ImplFD(handle)) {}

FD::FD(int fd):
  mp_impl(new ImplFD(reinterpret_cast<HANDLE>(_get_osfhandle(fd)))) {}

FD::FD(FD&& o) = default;
FD& FD::operator=(FD&& o) = default;

FD::~FD()
{
  if (mp_impl)
    close();
}

zsize_t FD::readAt(char* dest, zsize_t size, offset_t offset) const
{
  if (!mp_impl)
    return zsize_t(-1);
  EnterCriticalSection(&mp_impl->m_criticalSection);
  LARGE_INTEGER off;
  off.QuadPart = offset.v;
  if (!SetFilePointerEx(mp_impl->m_handle, off, NULL, FILE_BEGIN)) {
    goto err;
  }

  DWORD size_read;
  if (!ReadFile(mp_impl->m_handle, dest, size.v, &size_read, NULL)) {
    goto err;
  }
  if (size_read != size.v) {
    goto err;
  }
  LeaveCriticalSection(&mp_impl->m_criticalSection);
  return size;
err:
  LeaveCriticalSection(&mp_impl->m_criticalSection);
  return zsize_t(-1);
}

bool FD::seek(offset_t offset)
{
  if(!mp_impl)
    return false;
  LARGE_INTEGER off;
  off.QuadPart = offset.v;
  return SetFilePointerEx(mp_impl->m_handle, off, NULL, FILE_BEGIN);
}

zsize_t FD::getSize() const
{
  if(!mp_impl)
    return zsize_t(0);
  LARGE_INTEGER size;
  if (!GetFileSizeEx(mp_impl->m_handle, &size)) {
    size.QuadPart = 0;
  }
  return zsize_t(size.QuadPart);
}

int FD::release()
{
  if(!mp_impl)
    return -1;
  int ret = _open_osfhandle(reinterpret_cast<intptr_t>(mp_impl->m_handle), 0);
  mp_impl->m_handle = INVALID_HANDLE_VALUE;
  return ret;
}

bool FD::close()
{
  if (!mp_impl || mp_impl->m_handle == INVALID_HANDLE_VALUE) {
    return false;
  }
  return CloseHandle(mp_impl->m_handle);
}

std::unique_ptr<wchar_t[]> FS::toWideChar(path_t path)
{
  auto size = MultiByteToWideChar(CP_UTF8, 0,
                path.c_str(), -1, nullptr, 0);
  auto wdata = std::unique_ptr<wchar_t[]>(new wchar_t[size]);
  auto ret = MultiByteToWideChar(CP_UTF8, 0,
                path.c_str(), -1, wdata.get(), size);
  if (0 == ret) {
    std::ostringstream oss;
    oss << "Cannot convert path to wchar : " << GetLastError();
    throw std::runtime_error(oss.str());
  }
  return wdata;
}

FD FS::openFile(path_t filepath)
{
  auto wpath = toWideChar(filepath);
  FD::fd_t handle;
  handle = CreateFileW(wpath.get(),
             GENERIC_READ,
             FILE_SHARE_READ,
             NULL,
             OPEN_EXISTING,
             FILE_ATTRIBUTE_READONLY|FILE_FLAG_RANDOM_ACCESS,
             NULL);
  if (handle == INVALID_HANDLE_VALUE) {
    std::ostringstream oss;
    oss << "Cannot open file : " << GetLastError();
    throw std::runtime_error(oss.str());
  }
  return FD(handle);
}

bool FS::makeDirectory(path_t path)
{
  auto wpath = toWideChar(path);
  auto ret = CreateDirectoryW(wpath.get(), NULL);
  return ret;
}


void FS::rename(path_t old_path, path_t new_path)
{
  MoveFileW(toWideChar(old_path).get(), toWideChar(new_path).get());
}

std::string FS::join(path_t base, path_t name)
{
  return base + "\\" + name;
}

bool FS::removeDir(path_t path)
{
  return RemoveDirectoryW(toWideChar(path).get());
}

bool FS::removeFile(path_t path)
{
  return DeleteFileW(toWideChar(path).get());
}

}; // windows namespace

}; // zim namespace

