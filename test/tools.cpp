/*
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

#include "tools.h"

#ifdef _WIN32
#include <locale>
#include <codecvt>
#include <windows.h>
#include <fileapi.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>

namespace zim
{

namespace unittests
{

TempFile::TempFile(const char* name)
 : fd_(-1)
{
#ifdef _WIN32
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> utfConv;
  wchar_t cbase[MAX_PATH];
  const std::wstring wname = utfConv.from_bytes(name);
  GetTempPathW(MAX_PATH-(wname.size()+2), cbase);
  //This create a empty file, we just have to open it later
  GetTempFileNameW(cbase, wname.c_str(), 0, wpath_);
  path_ = utfConv.to_bytes(wpath_);
#else
  const char* const TMPDIR = std::getenv("TMPDIR");
  const std::string tmpdir(TMPDIR ? TMPDIR : "/tmp");
  path_ = tmpdir + "/" + name + "_XXXXXX";
  auto tmp_fd = mkstemp(&path_[0]);
  ::close(tmp_fd);
#endif
}

TempFile::~TempFile()
{
  close();
#ifdef _WIN32
  DeleteFileW(wpath_);
#else
  unlink(path_.c_str());
#endif
}

int TempFile::fd()
{
  if (fd_ == -1) {
#ifdef _WIN32
    fd_ = _wopen(wpath_, _O_RDWR);
#else
    fd_ = open(path_.c_str(), O_RDWR);
#endif
  }
  return fd_;
}

void TempFile::close()
{
  if (fd_ != -1) {
	::close(fd_);
	fd_ = -1;
  }
}

std::unique_ptr<TempFile>
makeTempFile(const char* name, const std::string& content)
{
  std::unique_ptr<TempFile> p(new TempFile(name));
  write(p->fd(), &content[0], content.size());
  p->close();
  return p;
}


} // namespace unittests

} // namespace zim
