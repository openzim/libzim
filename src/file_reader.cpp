/*
 * Copyright (C) 2017-2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#include <zim/zim.h>
#include <zim/error.h>
#include <zim/tools.h>
#include "file_reader.h"
#include "file_compound.h"
#include "buffer.h"
#include <errno.h>
#include <string.h>
#include <cstring>
#include <fcntl.h>
#include <system_error>
#include <algorithm>
#include <stdexcept>


#ifndef _WIN32
#  include <sys/mman.h>
#  include <unistd.h>
#endif

#if defined(_MSC_VER)
# include <io.h>
# include <BaseTsd.h>
  typedef SSIZE_T ssize_t;
#endif

namespace {
  [[noreturn]] void throwSystemError(const std::string& errorText)
  {
#ifdef _WIN32
    // Windows doesn't use errno.
    throw std::system_error(std::error_code(), errorText);
#else
    std::error_code ec(errno, std::generic_category());
    throw std::system_error(ec, errorText);
#endif
  }
}

namespace zim {

////////////////////////////////////////////////////////////////////////////////
// MultiPartFileReader
////////////////////////////////////////////////////////////////////////////////

MultiPartFileReader::MultiPartFileReader(std::shared_ptr<const FileCompound> source)
  : MultiPartFileReader(source, offset_t(0), source->fsize()) {}

MultiPartFileReader::MultiPartFileReader(std::shared_ptr<const FileCompound> source, offset_t offset, zsize_t size)
  : BaseFileReader(offset, size),
    source(source)
{
  ASSERT(offset.v, <=, source->fsize().v);
  ASSERT(offset.v+size.v, <=, source->fsize().v);
}

char MultiPartFileReader::readImpl(offset_t offset) const {
  offset += _offset;
  auto part_pair = source->locate(offset);
  auto& fhandle = part_pair->second->fhandle();
  offset_t logical_local_offset = offset - part_pair->first.min;
  ASSERT(logical_local_offset, <=, part_pair->first.max);
  offset_t physical_local_offset = logical_local_offset + part_pair->second->offset();
  char ret;
  try {
    fhandle.readAt(&ret, zsize_t(1), physical_local_offset);
  } catch (std::runtime_error& e) {
    //Error while reading.
    Formatter fmt;
    fmt << "Cannot read a char.\n";
    fmt << " - File part is " <<  part_pair->second->filename() << "\n";
    fmt << " - File part size is " << part_pair->second->size().v << "\n";
    fmt << " - File part range is " << part_pair->first.min << "-" << part_pair->first.max << "\n";
    fmt << " - Reading offset at " << offset.v << "\n";
    fmt << " - logical local offset is " << logical_local_offset.v << "\n";
    fmt << " - physical local offset is " << physical_local_offset.v << "\n";
    fmt << " - error is " << e.what() << "\n";
    throwSystemError(fmt);
  };
  return ret;
}

void MultiPartFileReader::readImpl(char* dest, offset_t offset, zsize_t size) const {
  offset += _offset;
  auto found_range = source->locate(offset, size);
  for(auto current = found_range.first; current!=found_range.second; current++){
    auto part = current->second;
    Range partRange = current->first;
    offset_t logical_local_offset = offset - partRange.min;
    ASSERT(size.v, >, 0U);
    zsize_t size_to_get = zsize_t(std::min(size.v, part->size().v-logical_local_offset.v));
    offset_t physical_local_offset = logical_local_offset + part->offset();
    try {
      part->fhandle().readAt(dest, size_to_get, physical_local_offset);
    } catch (std::runtime_error& e) {
      Formatter fmt;
      fmt << "Cannot read chars.\n";
      fmt << " - File part is " <<  part->filename() << "\n";
      fmt << " - File part size is " << part->size().v << "\n";
      fmt << " - File part range is " << partRange.min << "-" << partRange.max << "\n";
      fmt << " - size_to_get is " << size_to_get.v << "\n";
      fmt << " - total size is " << size.v << "\n";
      fmt << " - Reading offset at " << offset.v << "\n";
      fmt << " - logical local offset is " << logical_local_offset.v << "\n";
      fmt << " - physical local offset is " << physical_local_offset.v << "\n";
      fmt << " - error is " << e.what() << "\n";
      throwSystemError(fmt);
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
#if defined(__linux__)
  const auto MAP_FLAGS = MAP_PRIVATE|MAP_POPULATE;
#elif defined(__FreeBSD__)
  const auto MAP_FLAGS = MAP_PRIVATE|MAP_PREFAULT_READ;
#else
  const auto MAP_FLAGS = MAP_PRIVATE;
#endif

  const auto p = (char*)mmap(NULL, size, PROT_READ, MAP_FLAGS, fd, offset);
  if (p == MAP_FAILED) {
    // mmap may fails for a lot of reason.
    // Most of them (io error, too big size...) may not recoverable but some of
    // them may be relative to mmap only and a "simple" read from the file would work.
    // Let's throw a MMapException to fallback to read (and potentially fail again there).
    throw MMapException();
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

const Buffer BaseFileReader::get_buffer(offset_t offset, zsize_t size) const {
  ASSERT(size, <=, _size);
#ifdef ENABLE_USE_MMAP
  try {
    return get_mmap_buffer(offset, size);
  } catch(MMapException& e)
#endif
  {
    // We cannot do the mmap, for several possible reasons:
    // - Mmap offset is too big (>4GB on 32 bits)
    // - The range is several part
    // - We are on Windows.
    // - Mmap itself has failed
    // We will have to do some memory copies (or fail trying to) :/
    // [TODO] Use Windows equivalent for mmap.
    auto ret_buffer = Buffer::makeBuffer(size);
    read(const_cast<char*>(ret_buffer.data()), offset, size);
    return ret_buffer;
  }
}

const Buffer MultiPartFileReader::get_mmap_buffer(offset_t offset, zsize_t size) const {
#ifdef ENABLE_USE_MMAP
  auto found_range = source->locate(_offset + offset, size);
  auto first_part_containing_it = found_range.first;
  if (++first_part_containing_it != found_range.second) {
    throw MMapException();
  }

  // The range is in only one part
  auto range = found_range.first->first;
  auto part = found_range.first->second;
  auto logical_local_offset = offset + _offset - range.min;
  ASSERT(size, <=, part->size());
  int fd = part->fhandle().getNativeHandle();
  auto physical_local_offset = logical_local_offset + part->offset();
  return Buffer::makeBuffer(makeMmappedBuffer(fd, physical_local_offset, size), size);
#else
  return Buffer::makeBuffer(size); // unreachable
#endif
}

bool Reader::can_read(offset_t offset, zsize_t size) const
{
    return (offset.v <= this->size().v && (offset.v+size.v) <= this->size().v);
}


std::unique_ptr<const Reader> MultiPartFileReader::sub_reader(offset_t offset, zsize_t size) const
{
  ASSERT(offset.v+size.v, <=, _size.v);
  // TODO: can use a FileReader here if the new range fully belongs to a single part
  return std::unique_ptr<Reader>(new MultiPartFileReader(source, _offset+offset, size));
}

////////////////////////////////////////////////////////////////////////////////
// FileReader
////////////////////////////////////////////////////////////////////////////////

FileReader::FileReader(FileHandle fh, offset_t offset, zsize_t size)
  : BaseFileReader(offset, size)
    , _fhandle(fh)
{
}

char FileReader::readImpl(offset_t offset) const
{
  offset += _offset;
  char ret;
  try {
    _fhandle->readAt(&ret, zsize_t(1), offset);
  } catch (std::runtime_error& e) {
    //Error while reading.
    Formatter fmt;
    fmt << "Cannot read a char.\n";
    fmt << " - Reading offset at " << offset.v << "\n";
    fmt << " - error is " << e.what() << "\n";
    throwSystemError(fmt);
  };
  return ret;
}

void FileReader::readImpl(char* dest, offset_t offset, zsize_t size) const
{
  offset += _offset;
  try {
    _fhandle->readAt(dest, size, offset);
  } catch (std::runtime_error& e) {
    Formatter fmt;
    fmt << "Cannot read chars.\n";
    fmt << " - Reading offset at " << offset.v << "\n";
    fmt << " - size is " << size.v << "\n";
    fmt << " - error is " << e.what() << "\n";
    throwSystemError(fmt);
  };
}

const Buffer FileReader::get_mmap_buffer(offset_t offset, zsize_t size) const {
#ifdef ENABLE_USE_MMAP
  auto local_offset = offset + _offset;
  int fd = _fhandle->getNativeHandle();
  return Buffer::makeBuffer(makeMmappedBuffer(fd, local_offset, size), size);
#else
  return Buffer::makeBuffer(size); // unreachable
#endif
}

std::unique_ptr<const Reader>
FileReader::sub_reader(offset_t offset, zsize_t size) const
{
  ASSERT(offset.v+size.v, <=, _size.v);
  return std::unique_ptr<const Reader>(new FileReader(_fhandle, _offset + offset, size));
}

} // zim
