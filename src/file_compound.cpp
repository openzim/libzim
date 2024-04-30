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

#include "file_compound.h"

#include <errno.h>
#include <stdexcept>
#include <string.h>
#include <sys/stat.h>
#include <zim/tools.h>

#ifdef _WIN32
#  include <io.h>
#else
#  include <unistd.h>
#endif

namespace zim {

void FileCompound::addPart(FilePart* fpart)
{
  const Range newRange(offset_t(_fsize.v), offset_t((_fsize+fpart->size()).v));
  emplace(newRange, fpart);
  _fsize += fpart->size();
}

std::shared_ptr<FileCompound> FileCompound::openSinglePieceOrSplitZimFile(std::string filename) {
  std::shared_ptr<FileCompound> fileCompound;
  if (filename.size() > 6 && filename.substr(filename.size()-6) == ".zimaa") {
    filename.resize(filename.size()-2);
  } else {
  try {
      fileCompound = std::make_shared<FileCompound>(filename);
    } catch(...) { }
  }

  if ( !fileCompound ) {
    fileCompound = std::make_shared<FileCompound>(filename, FileCompound::MultiPartToken::Multi);
  }
  return fileCompound;
}

FileCompound::FileCompound(const std::string& filename):
  _filename(filename),
  _fsize(0)
{
  addPart(new FilePart(filename));
}

FileCompound::FileCompound(const std::string& base_filename, MultiPartToken _token):
  _filename(base_filename),
  _fsize(0)
{
  try {
    for (char ch0 = 'a'; ch0 <= 'z'; ++ch0)
    {
      const std::string fname0 = base_filename + ch0;
      for (char ch1 = 'a'; ch1 <= 'z'; ++ch1)
      {
        addPart(new FilePart(fname0 + ch1));
      }
    }
  } catch (std::runtime_error& e) {
    // This catch acts as a break for the double loop.
  }
  if (empty()) {
    // We haven't found any part
    throw std::runtime_error(Formatter() << "Error opening as a split file: " << base_filename);
  }
}

#ifndef _WIN32
FileCompound::FileCompound(int fd):
  _filename(),
  _fsize(0)
{
  addPart(new FilePart(fd));
}

FileCompound::FileCompound(FdInput fd):
  _filename(),
  _fsize(0)
{
  addPart(new FilePart(fd));
}

FileCompound::FileCompound(const std::vector<FdInput>& fds):
  _filename(),
  _fsize(0)
{
  for (auto& fd: fds) {
    addPart(new FilePart(fd));
  }
}
#endif

FileCompound::~FileCompound() {
  for(auto it=begin(); it!=end(); it++) {
    auto filepart = it->second;
    delete filepart;
  }
}

time_t FileCompound::getMTime() const {
  if (mtime || empty())
    return mtime;

  const char* fname = begin()->second->filename().c_str();

  #if defined(HAVE_STAT64) && ! defined(__APPLE__)
    struct stat64 st;
    int ret = ::stat64(fname, &st);
  #else
    struct stat st;
    int ret = ::stat(fname, &st);
  #endif
  if (ret != 0)
    throw std::runtime_error(Formatter() << "stat failed with errno " << errno
                                          << " : " << strerror(errno));

  mtime = st.st_mtime;

  return mtime;
}

} // zim
