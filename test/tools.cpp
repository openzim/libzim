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
#include <io.h>
#else
#include <dirent.h>
#endif

#include "../src/fs.h"

#include <fcntl.h>
#include <sys/stat.h>
#include "gtest/gtest.h"

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
    fd_ = _wopen(wpath_, _O_RDWR | _O_BINARY);
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

void setDataDir(std::string& dataDir)
{
  // FAIL must be used in a void function. So we need to use a out parameter.
  const char* cDataDir = std::getenv("ZIM_TEST_DATA_DIR");
  if (cDataDir == NULL) {
    dataDir = "INVALID_DATA_DIR";
    FAIL() << "ZIM_TEST_DATA_DIR is not defined. You must define it to the directory containing test zim files.";
  }
  dataDir = cDataDir;
}

TestFile::TestFile(const std::string& dataDir, const std::string& category, const std::string& filename) :
  filename(filename),
  category(category),
  path(zim::DEFAULTFS::join(zim::DEFAULTFS::join(dataDir, category), filename))
{
}

const std::vector<TestFile> getDataFilePath(const std::string& filename, const std::string& category)
{
  std::vector<TestFile> filePaths;
  std::string dataDirPath;
  setDataDir(dataDirPath);

  if (!category.empty()) {
      // We have asked for a particular category.
      filePaths.emplace_back(dataDirPath, category, filename);
  } else {
#ifdef _WIN32
    // We don't have dirent.h in windows.
    // If we move to test data out of the repository, we will need a way to discover the data.
    // Use a static list of categories for now.
    for (auto& category: {"normal", "nons"}) {
      filePaths.emplace_back(dataDirPath, category, filename);
    }
#else
    auto dataDir = opendir(dataDirPath.c_str());

    if (!dataDir) {
      filePaths.emplace_back(dataDirPath, "NO_DATA_DIR", filename);
      return filePaths;
    }
    struct dirent* current = NULL;
    while((current = readdir(dataDir))) {
      if (current->d_name[0] == '.' || current->d_name[0] == '_') {
        continue;
      }
      filePaths.emplace_back(dataDirPath, current->d_name, filename);
    }
    closedir(dataDir);
#endif
  }

  return filePaths;
}

} // namespace unittests

} // namespace zim
