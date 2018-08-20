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

#include "file_part.h"
#include "tools.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <cstdlib>
#include <iostream>
#ifndef _WIN32
#  include <unistd.h>
#else
#  include <io.h>
#endif

namespace zim {

FilePart::FilePart(const std::string& filename)
  : filename_(filename),
    size_(0)
{
  fd_ = openFile(filename);
  if ( fd_ >= 0 ) {
#ifdef _WIN32
    size_.v = _filelengthi64(fd_);
#else
    struct stat sb;
    fstat(fd_, &sb);
    size_.v = sb.st_size;
#endif
  }
}

FilePart::FilePart(std::FILE* filestream)
  : filename_(""),
    size_(0)
{
  fd_ = fileno(filestream);
  if ( fd_ >= 0 ) {
#ifdef _WIN32
    size_.v = _filelengthi64(fd_);
#else
    struct stat sb;
    fstat(fd_, &sb);
    size_.v = sb.st_size;
#endif
  }
}

FilePart::~FilePart() {
#ifdef _WIN32
  _close(fd_);
#else
  close(fd_);
#endif
}


} // zim
