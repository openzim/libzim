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

#include <zim/buffer.h>

#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>

#if !defined(_WIN32)
#include <sys/mman.h>
#endif

namespace zim {

std::shared_ptr<Buffer> Buffer::sub_buffer(std::size_t offset, std::size_t size)
{
  return std::make_shared<SubBuffer>(shared_from_this(), offset, size);
}

#if !defined(_WIN32)
MMapBuffer::MMapBuffer(const std::string& filename, std::size_t offset, std::size_t size):
  Buffer(size)
{
  fd = open(filename.c_str(), O_RDONLY);
  struct stat filesize;
  fstat(fd, &filesize);
  std::size_t pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
  _offset = offset-pa_offset;
  assert(pa_offset < filesize.st_size);
  assert(offset+size <= filesize.st_size);

  _data = (char*)mmap(NULL, size + _offset, PROT_READ, MAP_PRIVATE, fd, pa_offset);
}

MMapBuffer::~MMapBuffer()
{
  munmap(_data, size_ + _offset);
  close(fd);
}

#endif

} //zim
