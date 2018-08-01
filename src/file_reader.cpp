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
#include "config.h"
#include "envvalue.h"
#include <errno.h>
#include <string.h>
#include <cstring>
#include <fcntl.h>
#include <lzma.h>
#include <pthread.h>
#include <sstream>
#include <system_error>


#if defined(_MSC_VER)
# include <io.h>
# include <BaseTsd.h>
  typedef SSIZE_T ssize_t;
#endif

#if defined(ENABLE_ZLIB)
#include <zlib.h>
#endif

namespace zim {

#if !defined(_WIN32)
static ssize_t read_at(int fd, char* dest, zsize_t size, offset_t offset)
{
  ssize_t full_size_read = 0;
  auto size_to_read = size.v;
  auto current_offset = offset.v;
  errno = 0;
  while (size_to_read > 0) {
    auto size_read = pread(fd, dest, size_to_read, current_offset);
    if (size_read == -1) {
      return -1;
    }
    size_to_read -= size_read;
    current_offset += size_read;
    full_size_read += size_read;
  }
  return full_size_read;
}
#else
static ssize_t read_at(int fd, char* dest, zsize_t size, offset_t offset)
{
  // [TODO] We are locking all fd at the same time here.
  // We should find a way to have a lock per fd.
  errno = 0;
  static pthread_mutex_t fd_lock = PTHREAD_MUTEX_INITIALIZER;
  if (offset.v > static_cast<uint64_t>(INT64_MAX)) {
    return -1;
  }
  pthread_mutex_lock(&fd_lock);
  if (_lseeki64(fd, offset.v, SEEK_SET) != static_cast<int64_t>(offset.v)) {
    pthread_mutex_unlock(&fd_lock);
    return -1;
  }
  ssize_t full_size_read = 0;
  auto size_to_read = size.v;
  while (size_to_read > 0) {
    unsigned int s_to_read = 0;
    if (size_to_read > UINT_MAX) {
      s_to_read = UINT_MAX;
    } else {
      s_to_read = size_to_read;
    }
    auto size_read = _read(fd, dest, s_to_read);
    if (size_read == -1) {
      pthread_mutex_unlock(&fd_lock);
      return -1;
    }
    size_to_read -= size_read;
    full_size_read += size_read;
  }
  pthread_mutex_unlock(&fd_lock);
  return full_size_read;
}
#endif

FileReader::FileReader(std::shared_ptr<const FileCompound> source)
  : FileReader(source, offset_t(0), source->fsize()) {}

FileReader::FileReader(std::shared_ptr<const FileCompound> source, offset_t offset)
  : FileReader(source, offset, zsize_t(source->fsize().v-offset.v)) {}

FileReader::FileReader(std::shared_ptr<const FileCompound> source, offset_t offset, zsize_t size)
  : source(source),
    _offset(offset),
    _size(size)
{
  ASSERT(offset.v, <, source->fsize().v);
  ASSERT(offset.v+size.v, <=, source->fsize().v);
}

char FileReader::read(offset_t offset) const {
  ASSERT(offset.v, <, _size.v);
  offset += _offset;
  auto part_pair = source->lower_bound(offset);
  int fd = part_pair->second->fd();
  offset_t local_offset = offset - part_pair->first.min;
  ASSERT(local_offset, <=, part_pair->first.max);
  char ret;
  auto read_ret = read_at(fd, &ret, zsize_t(1), local_offset);
  if (read_ret != 1) {
    //Error while reading.
    std::ostringstream s;
    s << "Cannot read a char.\n";
    s << " - File part is " <<  part_pair->second->filename() << "\n";
    s << " - File part size is " << part_pair->second->size().v << "\n";
    s << " - File part range is " << part_pair->first.min << "-" << part_pair->first.max << "\n";
    s << " - Reading offset at " << offset.v << "\n";
    s << " - local offset is " << local_offset.v << "\n";
    s << " - read return is " << read_ret << "\n";
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
    FilePart* part = current->second;
    Range partRange = current->first;
    offset_t local_offset = offset-partRange.min;
    ASSERT(size.v, >, 0U);
    zsize_t size_to_get = zsize_t(std::min(size.v, part->size().v-local_offset.v));
    int fd = part->fd();
    auto read_ret = read_at(fd, dest, size_to_get, local_offset);
    if (read_ret != static_cast<int64_t>(size_to_get.v)) {
      std::ostringstream s;
      s << "Cannot read chars.\n";
      s << " - File part is " <<  part->filename() << "\n";
      s << " - File part size is " << part->size().v << "\n";
      s << " - File part range is " << partRange.min << "-" << partRange.max << "\n";
      s << " - size_to_get is " << size_to_get.v << "\n";
      s << " - total size is " << size.v << "\n";
      s << " - Reading offset at " << offset.v << "\n";
      s << " - local offset is " << local_offset.v << "\n";
      s << " - read return is " << read_ret << "\n";
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
  auto found_range = source->locate(_offset+offset, size);
  auto first_part_containing_it = found_range.first;
  if (++first_part_containing_it == found_range.second) {
    // The range is in only one part
    auto range = found_range.first->first;
    auto part = found_range.first->second;
    auto local_offset = offset + _offset - range.min;
    ASSERT(size, <=, part->size());
    int fd = part->fd();
    auto buffer = std::shared_ptr<const Buffer>(new MMapBuffer(fd, local_offset, size));
    return buffer;
  } else
#endif
  {
    // The range is several part, or we are on Windows.
    // We will have to do some memory copies :/
    // [TODO] Use Windows equivalent for mmap.
    char* p = new char[size.v];
    auto ret_buffer = std::shared_ptr<const Buffer>(new MemoryBuffer<true>(p, size));
    read(p, offset, size);
    return ret_buffer;
  }
}

bool Reader::can_read(offset_t offset, zsize_t size)
{
    return (offset.v <= this->size().v && (offset.v+size.v) <= this->size().v);
}

char* lzma_uncompress(const char* raw_data, zsize_t raw_size, zsize_t* dest_size) {
  // We don't know what will be the result size.
  // Let's assume it will be something like the minChunkSize used at creation
  zsize_t _dest_size = zsize_t(1024*1024);
  char* ret_data = new char[_dest_size.v];
  lzma_stream stream = LZMA_STREAM_INIT;
  unsigned memsize = envMemSize("ZIM_LZMA_MEMORY_SIZE", LZMA_MEMORY_SIZE * 1024 * 1024);
  auto errcode = lzma_stream_decoder(&stream, memsize, 0);
  if (errcode != LZMA_OK) {
    throw std::runtime_error("Impossible to allocated needed memory to uncompress lzma stream");
  }

  stream.next_in = (const unsigned char*)raw_data;
  stream.avail_in = raw_size.v;
  stream.next_out = (unsigned char*) ret_data;
  stream.avail_out = _dest_size.v;
  do {
    errcode = lzma_code(&stream, LZMA_FINISH);
    if (errcode == LZMA_BUF_ERROR) {
      if (stream.avail_in == 0 && stream.avail_out != 0)  {
        // End of input stream.
        // lzma haven't recognize the end of the input stream but there is no
        // more input.
        // As we know that we should have all the input stream, it is probably
        // because the stream has not been close correctly at zim creation.
        // It means that the lzma stream is not full and this is an error in the
        // zim file.
      } else {
        //Not enought output size
        _dest_size.v *= 2;
        char * new_ret_data = new char[_dest_size.v];
        memcpy(new_ret_data, ret_data, stream.total_out);
        stream.next_out = (unsigned char*)(new_ret_data + stream.total_out);
        stream.avail_out = _dest_size.v - stream.total_out;
        delete [] ret_data;
        ret_data = new_ret_data;
        continue;
      }
    }
    if (errcode != LZMA_STREAM_END && errcode != LZMA_OK) {
      throw ZimFileFormatError("Invalid lzma stream for cluster.");
    }
  } while (errcode != LZMA_STREAM_END);
  dest_size->v = stream.total_out;
  lzma_end(&stream);
  return ret_data;
}

#if defined(ENABLE_ZLIB)
char* zip_uncompress(const char* raw_data, zsize_t raw_size, zsize_t* dest_size) {
  zsize_t _dest_size = zsize_t(1024*1024);
  char* ret_data = new char[_dest_size.v];

  z_stream stream;
  memset(&stream, 0, sizeof(stream));

  stream.next_in = (unsigned char*) raw_data;
  stream.avail_in = raw_size.v;
  stream.next_out = (unsigned char*) ret_data;
  stream.avail_out = _dest_size.v;
  auto errcode = ::inflateInit(&stream);
  if (errcode != Z_OK) {
    throw std::runtime_error("Impossible to allocated needed memory to uncompress zlib stream");
  }
  do {

    errcode = ::inflate(&stream, Z_FINISH);
    if (errcode == Z_BUF_ERROR ) {
      if (stream.avail_in == 0 && stream.avail_out != 0)  {
        // End of input stream.
        // zlib haven't recognize the end of the input stream but there is no
        // more input.
        // As we know that we should have all the input stream, it is probably
        // because the stream has not been close correctly at zim creation.
        // It means that the zlib stream is not full and this is an error in the
        // zim file.
      } else {
        //Not enought output size
        _dest_size.v *= 2;
        char * new_ret_data = new char[_dest_size.v];
        memcpy(new_ret_data, ret_data, stream.total_out);
        stream.next_out = (unsigned char*)(new_ret_data + stream.total_out);
        stream.avail_out = _dest_size.v - stream.total_out;
        delete [] ret_data;
        ret_data = new_ret_data;
        continue;
     }
    }
    if (errcode != Z_STREAM_END && errcode != Z_OK) {
      throw ZimFileFormatError("Invalid zlib stream for cluster.");
    }
  } while ( errcode != Z_STREAM_END );
  dest_size->v = stream.total_out;
  ::inflateEnd(&stream);
  return ret_data;
}
#endif

std::shared_ptr<const Buffer> Reader::get_clusterBuffer(offset_t offset, zsize_t size, CompressionType comp) const
{
  auto raw_buffer = get_buffer(offset, size);
  zsize_t uncompressed_size(0);
  char* uncompressed_data = nullptr;
  switch (comp) {
    case zimcompLzma:
      uncompressed_data = lzma_uncompress(raw_buffer->data(), size, &uncompressed_size);
      break;
    case zimcompZip:
#if defined(ENABLE_ZLIB)
      uncompressed_data = zip_uncompress(raw_buffer->data(), size, &uncompressed_size);
#else
      throw std::runtime_error("zlib not enabled in this library");
#endif
      break;
    default:
      throw std::logic_error("compressions should not be something else than zimcompLzma or zimComZip.");
  }
  return std::shared_ptr<const Buffer>(new MemoryBuffer<true>(uncompressed_data, uncompressed_size));
}

std::unique_ptr<const Reader> Reader::sub_clusterReader(offset_t offset, zsize_t size, CompressionType* comp, bool* extended) const {
  uint8_t clusterInfo = read(offset);
  *comp = static_cast<CompressionType>(clusterInfo & 0x0F);
  *extended = clusterInfo & 0x10;

  switch (*comp) {
    case zimcompDefault:
    case zimcompNone:
      {
      // No compression, just a sub_reader
        return sub_reader(offset+offset_t(1), size-zsize_t(1));
      }
      break;
    case zimcompLzma:
    case zimcompZip:
      {
        auto buffer = get_clusterBuffer(offset+offset_t(1), size-zsize_t(1), *comp);
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
