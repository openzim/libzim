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

#include <zim/zim.h>
#include <zim/error.h>
#include "file_reader.h"
#include "file_compound.h"
#include "buffer.h"
#include <errno.h>
#include <string.h>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <system_error>
#include <algorithm>


#ifndef _WIN32
#  include <sys/mman.h>
#  include <unistd.h>
#endif

#if defined(_MSC_VER)
# include <io.h>
# include <BaseTsd.h>
  typedef SSIZE_T ssize_t;
#endif

namespace zim {

MultiPartFileReader::MultiPartFileReader(std::shared_ptr<const FileCompound> source)
  : MultiPartFileReader(source, offset_t(0), source->fsize()) {}

MultiPartFileReader::MultiPartFileReader(std::shared_ptr<const FileCompound> source, offset_t offset)
  : MultiPartFileReader(source, offset, zsize_t(source->fsize().v-offset.v)) {}

MultiPartFileReader::MultiPartFileReader(std::shared_ptr<const FileCompound> source, offset_t offset, zsize_t size)
  : source(source),
    _offset(offset),
    _size(size)
{
  ASSERT(offset.v, <=, source->fsize().v);
  ASSERT(offset.v+size.v, <=, source->fsize().v);
}

char MultiPartFileReader::read(offset_t offset) const {
  ASSERT(offset.v, <, _size.v);
  offset += _offset;
  auto part_pair = source->locate(offset);
  auto& fhandle = part_pair->second->fhandle();
  offset_t local_offset = offset - part_pair->first.min;
  ASSERT(local_offset, <=, part_pair->first.max);
  char ret;
  try {
    fhandle.readAt(&ret, zsize_t(1), local_offset);
  } catch (std::runtime_error& e) {
    //Error while reading.
    std::ostringstream s;
    s << "Cannot read a char.\n";
    s << " - File part is " <<  part_pair->second->filename() << "\n";
    s << " - File part size is " << part_pair->second->size().v << "\n";
    s << " - File part range is " << part_pair->first.min << "-" << part_pair->first.max << "\n";
    s << " - Reading offset at " << offset.v << "\n";
    s << " - local offset is " << local_offset.v << "\n";
    s << " - error is " << strerror(errno) << "\n";
    std::error_code ec(errno, std::generic_category());
    throw std::system_error(ec, s.str());
  };
  return ret;
}


void MultiPartFileReader::read(char* dest, offset_t offset, zsize_t size) const {
  ASSERT(offset.v, <, _size.v);
  ASSERT(offset.v+size.v, <=, _size.v);
  if (! size ) {
    return;
  }
  offset += _offset;
  auto found_range = source->locate(offset, size);
  for(auto current = found_range.first; current!=found_range.second; current++){
    auto part = current->second;
    Range partRange = current->first;
    offset_t local_offset = offset-partRange.min;
    ASSERT(size.v, >, 0U);
    zsize_t size_to_get = zsize_t(std::min(size.v, part->size().v-local_offset.v));
    try {
      part->fhandle().readAt(dest, size_to_get, local_offset);
    } catch (std::runtime_error& e) {
      std::ostringstream s;
      s << "Cannot read chars.\n";
      s << " - File part is " <<  part->filename() << "\n";
      s << " - File part size is " << part->size().v << "\n";
      s << " - File part range is " << partRange.min << "-" << partRange.max << "\n";
      s << " - size_to_get is " << size_to_get.v << "\n";
      s << " - total size is " << size.v << "\n";
      s << " - Reading offset at " << offset.v << "\n";
      s << " - local offset is " << local_offset.v << "\n";
      s << " - error is " << strerror(errno) << "\n";
      std::error_code ec(errno, std::generic_category());
      throw std::system_error(ec, s.str());
    };
    ASSERT(size_to_get, <=, size);
    dest += size_to_get.v;
    size -= size_to_get;
    offset += size_to_get;
  }
  ASSERT(size.v, ==, 0U);
}

#ifdef ENABLE_USE_MMAP
namespace
{

class MMapException : std::exception {};

char*
mmapReadOnly(int fd, offset_type offset, size_type size)
{
#if defined(__APPLE__) || defined(__OpenBSD__)
  const auto MAP_FLAGS = MAP_PRIVATE;
#elif defined(__FreeBSD__)
  const auto MAP_FLAGS = MAP_PRIVATE|MAP_PREFAULT_READ;
#else
  const auto MAP_FLAGS = MAP_PRIVATE|MAP_POPULATE;
#endif

  const auto p = (char*)mmap(NULL, size, PROT_READ, MAP_FLAGS, fd, offset);
  if (p == MAP_FAILED )
  {
    std::ostringstream s;
    s << "Cannot mmap size " << size << " at off " << offset
      << " : " << strerror(errno);
    throw std::runtime_error(s.str());
  }
  return p;
}

Buffer::DataPtr
makeMmappedBuffer(int fd, offset_t offset, zsize_t size)
{
  const offset_type pageAlignedOffset(offset.v & ~(sysconf(_SC_PAGE_SIZE) - 1));
  const size_t alignmentAdjustment = offset.v - pageAlignedOffset;
  size += alignmentAdjustment;

#if !MMAP_SUPPORT_64
  if(pageAlignedOffset >= INT32_MAX) {
    throw MMapException();
  }
#endif
  char* const mmappedAddress = mmapReadOnly(fd, pageAlignedOffset, size.v);
  const auto munmapDeleter = [mmappedAddress, size](char* ) {
                               munmap(mmappedAddress, size.v);
                             };

  return Buffer::DataPtr(mmappedAddress+alignmentAdjustment, munmapDeleter);
}

} // unnamed namespace
#endif // ENABLE_USE_MMAP

const Buffer MultiPartFileReader::get_buffer(offset_t offset, zsize_t size) const {
  ASSERT(size, <=, _size);
#ifdef ENABLE_USE_MMAP
  try {
    auto found_range = source->locate(_offset+offset, size);
    auto first_part_containing_it = found_range.first;
    if (++first_part_containing_it != found_range.second) {
      throw MMapException();
    }

    // The range is in only one part
    auto range = found_range.first->first;
    auto part = found_range.first->second;
    auto local_offset = offset + _offset - range.min;
    ASSERT(size, <=, part->size());
    int fd = part->fhandle().getNativeHandle();
    return Buffer::makeBuffer(makeMmappedBuffer(fd, local_offset, size), size);
  } catch(MMapException& e)
#endif
  {
    // The range is several part, or we are on Windows.
    // We will have to do some memory copies :/
    // [TODO] Use Windows equivalent for mmap.
    auto ret_buffer = Buffer::makeBuffer(size);
    read(const_cast<char*>(ret_buffer.data()), offset, size);
    return ret_buffer;
  }
}

bool Reader::can_read(offset_t offset, zsize_t size) const
{
    return (offset.v <= this->size().v && (offset.v+size.v) <= this->size().v);
}


std::unique_ptr<const Reader> MultiPartFileReader::sub_reader(offset_t offset, zsize_t size) const
{
  ASSERT(size, <=, _size);
  return std::unique_ptr<Reader>(new MultiPartFileReader(source, _offset+offset, size));
}

} // zim
