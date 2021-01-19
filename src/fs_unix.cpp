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

#include "fs_unix.h"
#include <stdexcept>
#include <vector>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

namespace zim
{

namespace unix {

zsize_t FD::readAt(char* dest, zsize_t size, offset_t offset) const
{
#if defined(__APPLE__) || defined(__OpenBSD__) || defined(__FreeBSD__)
# define PREAD pread
#else
# define PREAD pread64
#endif
  ssize_t full_size_read = 0;
  auto size_to_read = size.v;
  auto current_offset = offset.v;
  errno = 0;
  while (size_to_read > 0) {
    auto size_read = PREAD(m_fd, dest, size_to_read, current_offset);
    if (size_read == -1) {
      return zsize_t(-1);
    }
    size_to_read -= size_read;
    current_offset += size_read;
    full_size_read += size_read;
  }
  return zsize_t(full_size_read);
#undef PREAD
}

zsize_t FD::getSize() const
{
  struct stat sb;
  fstat(m_fd, &sb);
  return zsize_t(sb.st_size);
}

bool FD::seek(offset_t offset)
{
    return static_cast<int64_t>(offset.v) == lseek(m_fd, offset.v, SEEK_SET);
}

bool FD::close() {
  if (m_fd != -1) {
    return ::close(m_fd);
  }
  return -1;
}

FD FS::openFile(path_t filepath)
{
  int fd = open(filepath.c_str(), O_RDONLY);
  if (fd == -1) {
    throw std::runtime_error("");
  }
  return FD(fd);
}

bool FS::makeDirectory(path_t path)
{
  return !mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

void FS::rename(path_t old_path, path_t new_path)
{
  ::rename(old_path.c_str(), new_path.c_str());
}

std::string FS::join(path_t base, path_t name)
{
  return base + "/" + name;
}

bool FS::remove(path_t path)
{
  DIR* dir;
  /* It's a directory, remove all its entries first */
  if ((dir = opendir(path.c_str())) != NULL) {
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
      std::string childName = ent->d_name;
      if (childName !=  "." && childName != "..") {
        auto childPath = join(path, childName);
        remove(childPath);
      }
    }
    closedir(dir);
    return removeDir(path);
  }

  /* It's a file */
  else {
    return removeFile(path);
  }
}

bool FS::removeDir(path_t path) {
  return rmdir(path.c_str());
}

bool FS::removeFile(path_t path) {
  return ::remove(path.c_str());
}


}; // unix namespace

std::string getFilePathFromFD(int fd)
{
  std::ostringstream oss;
  oss << "/dev/fd/" << fd;

  return oss.str();
}

}; // zim namespace

