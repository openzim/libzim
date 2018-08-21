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

#include <synchapi.h>
#include <io.h>

namespace zim {

namespace windows {

FD::FD(int fd):
  m_handle(reinterpret_cast<HANDLE>(_get_osfhandle(fd)))
{
  InitializeCriticalSection(&m_criticalSection);
}

zsize_t FD::readAt(char* dest, zsize_t size, offset_t offset) const
{
  EnterCriticalSection(&m_criticalSection);
  LARGE_INTEGER off;
  off.QuadPart = offset.v;
  if (!SetFilePointerEx(m_handle, off, NULL, FILE_BEGIN)) {
    goto err;
  }

  DWORD size_read;
  if (!ReadFile(m_handle, dest, size.v, &size_read, NULL)) {
    goto err;
  }
  if (size_read != size.v) {
    goto err;
  }
  LeaveCriticalSection(&m_criticalSection);
  return size;
err:
  LeaveCriticalSection(&m_criticalSection);
  return zsize_t(-1);
}

bool FD::seek(offset_t offset)
{
  LARGE_INTEGER off;
  off.QuadPart = offset.v;
  return SetFilePointerEx(m_handle, off, NULL, FILE_BEGIN);
}

zsize_t FD::getSize() const
{
  LARGE_INTEGER size;
  if (!GetFileSizeEx(m_handle, &size)) {
    size.QuadPart = 0;
  }
  return zsize_t(size.QuadPart);
}

int FD::release()
{
  int ret = _open_osfhandle(reinterpret_cast<intptr_t>(m_handle), 0);
  m_handle = INVALID_HANDLE_VALUE;
  return ret;
}

bool FD::close()
{
  if (m_handle == INVALID_HANDLE_VALUE) {
    return false;
  }
  return CloseHandle(m_handle);
}

std::unique_ptr<wchar_t[]> FS::toWideChar(path_t path)
{
  auto size = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED,
                path.data(), path.size(), nullptr, 0);
  auto wdata = std::unique_ptr<wchar_t[]>(new wchar_t[size+1]);
  auto ret = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED,
                path.data(), path.size(), wdata.get(), size);
  if (0 == ret) {
    throw std::runtime_error("Cannot convert path to wchar");
  }
  wdata.get()[size] = 0;
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
    throw std::runtime_error("");
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

