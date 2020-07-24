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
#include "cluster.h"
#include "buffer.h"
#include "compression.h"
#include <errno.h>
#include <string.h>
#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <sstream>
#include <system_error>
#include <algorithm>


#if defined(_MSC_VER)
# include <io.h>
# include <BaseTsd.h>
  typedef SSIZE_T ssize_t;
#endif

namespace zim {

FileReader::FileReader(std::shared_ptr<const FileCompound> source)
  : FileReader(source, offset_t(0), source->fsize()) {}

FileReader::FileReader(std::shared_ptr<const FileCompound> source, offset_t offset)
  : FileReader(source, offset, zsize_t(source->fsize().v-offset.v)) {}

FileReader::FileReader(std::shared_ptr<const FileCompound> source, offset_t offset, zsize_t size)
  : source(source),
    _offset(offset),
    _size(size)
{
  ASSERT(offset.v, <=, source->fsize().v);
  ASSERT(offset.v+size.v, <=, source->fsize().v);
}

char FileReader::read(offset_t offset) const {
  ASSERT(offset.v, <, _size.v);
  offset += _offset;
  auto part_pair = source->lower_bound(offset);
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


void FileReader::read(char* dest, offset_t offset, zsize_t size) const {
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


std::shared_ptr<const Buffer> FileReader::get_buffer(offset_t offset, zsize_t size) const {
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
    return std::make_shared<MMapBuffer>(fd, local_offset, size);
  } catch(MMapException& e)
#endif
  {
    // The range is several part, or we are on Windows.
    // We will have to do some memory copies :/
    // [TODO] Use Windows equivalent for mmap.
    char* p = new char[size.v];
    auto ret_buffer = std::make_shared< MemoryBuffer<true> >(p, size);
    read(p, offset, size);
    return ret_buffer;
  }
}

bool Reader::can_read(offset_t offset, zsize_t size)
{
    return (offset.v <= this->size().v && (offset.v+size.v) <= this->size().v);
}


std::shared_ptr<const Buffer> Reader::get_clusterBuffer(offset_t offset, CompressionType comp) const
{
  zsize_t uncompressed_size(0);
  std::unique_ptr<char[]> uncompressed_data;
  switch (comp) {
    case zimcompLzma:
      uncompressed_data = uncompress<LZMA_INFO>(this, offset, &uncompressed_size);
      break;
    case zimcompZip:
#if defined(ENABLE_ZLIB)
      uncompressed_data = uncompress<ZIP_INFO>(this, offset, &uncompressed_size);
#else
      throw std::runtime_error("zlib not enabled in this library");
#endif
      break;
    case zimcompZstd:
      uncompressed_data = uncompress<ZSTD_INFO>(this, offset, &uncompressed_size);
      break;
    default:
      throw std::logic_error("compressions should not be something else than zimcompLzma, zimComZip or zimcompZstd.");
  }
  return std::make_shared< MemoryBuffer<true> >(uncompressed_data.release(), uncompressed_size);
}

std::unique_ptr<const Reader> Reader::sub_clusterReader(offset_t offset, CompressionType* comp, bool* extended) const {
  uint8_t clusterInfo = read(offset);
  *comp = static_cast<CompressionType>(clusterInfo & 0x0F);
  *extended = clusterInfo & 0x10;

  switch (*comp) {
    case zimcompDefault:
    case zimcompNone:
      {
        auto size = Cluster::read_size(this, *extended, offset + offset_t(1));
      // No compression, just a sub_reader
        return sub_reader(offset+offset_t(1), size);
      }
      break;
    case zimcompLzma:
    case zimcompZip:
    case zimcompZstd:
      {
        auto buffer = get_clusterBuffer(offset+offset_t(1), *comp);
        return std::unique_ptr<Reader>(new BufferReader(buffer));
      }
      break;
    case zimcompBzip2:
      throw std::runtime_error("bzip2 not enabled in this library");
    default:
      throw ZimFileFormatError("Invalid compression flag");
  }
}

std::unique_ptr<const Reader> FileReader::sub_reader(offset_t offset, zsize_t size) const
{
  ASSERT(size, <=, _size);
  return std::unique_ptr<Reader>(new FileReader(source, _offset+offset, size));
}


//BufferReader::BufferReader(std::shared_ptr<Buffer> source)
//  : source(source) {}

std::shared_ptr<const Buffer> BufferReader::get_buffer(offset_t offset, zsize_t size) const
{
  return source->sub_buffer(offset, size);
}

std::unique_ptr<const Reader> BufferReader::sub_reader(offset_t offset, zsize_t size) const
{
  //auto source_addr = source->data(0);
  auto sub_buff = get_buffer(offset, size);
  //auto buff_addr = sub_buff->data(0);
  std::unique_ptr<const Reader> sub_read(new BufferReader(sub_buff));
  return sub_read;
}

zsize_t BufferReader::size() const
{
  return source->size();
}

offset_t BufferReader::offset() const
{
  return offset_t((offset_type)(static_cast<const void*>(source->data(offset_t(0)))));
}


void BufferReader::read(char* dest, offset_t offset, zsize_t size) const {
  ASSERT(offset.v, <, source->size().v);
  ASSERT(offset+offset_t(size.v), <=, offset_t(source->size().v));
  if (! size ) {
    return;
  }
  memcpy(dest, source->data(offset), size.v);
}


char BufferReader::read(offset_t offset) const {
  ASSERT(offset.v, <, source->size().v);
  char dest;
  dest = *source->data(offset);
  return dest;
}


} // zim
