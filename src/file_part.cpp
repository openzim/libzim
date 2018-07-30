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

#include <fcntl.h>
#include <sys/stat.h>
#include <cstdlib>
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
  fd_ = open(filename.c_str(), O_RDONLY);
  struct stat sb;
  if ( fd_ >= 0 ) {
    fstat(fd_, &sb);
    size_.v = sb.st_size;
  }
}

FilePart::FilePart(std::FILE* filestream)
  : filename_(""),
    size_(0)
{
  fd_ = fileno(filestream);
  struct stat sb;
  if ( fd_ >= 0 ) {
    fstat(fd_, &sb);
    size_.v = sb.st_size;
  }
}

FilePart::~FilePart() {
  close(fd_);
}


} // zim
