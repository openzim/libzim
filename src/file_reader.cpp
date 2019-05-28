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
#include <algorithm>


#if defined(_MSC_VER)
# include <io.h>
# include <BaseTsd.h>
  typedef SSIZE_T ssize_t;
#endif

#if defined(ENABLE_ZLIB)
#include <zlib.h>
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
  ASSERT(offset.v, <, source->fsize().v);
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
    auto buffer = std::shared_ptr<const Buffer>(new MMapBuffer(fd, local_offset, size));
    return buffer;
  } catch(MMapException& e)
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

enum UNCOMP_ERROR {
  OK,
  STREAM_END,
  BUF_ERROR,
  OTHER
};

struct LZMA_INFO {
  typedef lzma_stream stream_t;
  static const std::string name;

  static void init_stream(stream_t* stream, char* raw_data) {
    *stream = LZMA_STREAM_INIT;
    unsigned memsize = envMemSize("ZIM_LZMA_MEMORY_SIZE", LZMA_MEMORY_SIZE * 1024 * 1024);
    auto errcode = lzma_stream_decoder(stream, memsize, 0);
    if (errcode != LZMA_OK) {
      throw std::runtime_error("Impossible to allocated needed memory to uncompress lzma stream");
    }
  }

  static UNCOMP_ERROR stream_run(stream_t* stream) {
    auto errcode = lzma_code(stream, LZMA_RUN);
    if (errcode == LZMA_BUF_ERROR)
      return BUF_ERROR;
    if (errcode == LZMA_STREAM_END)
      return STREAM_END;
    if (errcode == LZMA_OK)
      return OK;
    return OTHER;
  }

  static void stream_end(stream_t* stream) {
    lzma_end(stream);
  }
};
const std::string LZMA_INFO::name = "lzma";

#if defined(ENABLE_ZLIB)
struct ZIP_INFO {
  typedef z_stream stream_t;
  static const std::string name;

  static void init_stream(stream_t* stream, char* raw_data) {
    memset(stream, 0, sizeof(stream_t));
    stream->next_in = (unsigned char*) raw_data;
    stream->avail_in = 1024;
    auto errcode = ::inflateInit(stream);
    if (errcode != Z_OK) {
      throw std::runtime_error("Impossible to allocated needed memory to uncompress zlib stream");
    }
  }

  static UNCOMP_ERROR stream_run(stream_t* stream) {
    auto errcode = ::inflate(stream, Z_SYNC_FLUSH);
    if (errcode == Z_BUF_ERROR)
      return BUF_ERROR;
    if (errcode == Z_STREAM_END)
      return STREAM_END;
    if (errcode == Z_OK)
      return OK;
    return OTHER;
  }

  static void stream_end(stream_t* stream) {
    ::inflateEnd(stream);
  }
};
const std::string ZIP_INFO::name = "zlib";
#endif


#define CHUNCK_SIZE ((zim::size_type)1024)
template<typename INFO>
char* uncompress(const Reader* reader, offset_t startOffset, zsize_t* dest_size) {
  // We don't know the result size, neither the compressed size.
  // So we have to do chunk by chunk until decompressor is happy.
  // Let's assume it will be something like the minChunkSize used at creation
  zsize_t _dest_size = zsize_t(1024*1024);
  char* ret_data = new char[_dest_size.v];
  // The input is a buffer of CHUNCK_SIZE char max. It may be less if the last chunk
  // is at the end of the reader and the reader size is not a multiple of CHUNCK_SIZE.
  std::vector<char> raw_data(CHUNCK_SIZE);

  typename INFO::stream_t stream;
  INFO::init_stream(&stream, raw_data.data());

  zim::size_type availableSize = reader->size().v - startOffset.v;
  zim::size_type inputSize = std::min(availableSize, CHUNCK_SIZE);
  reader->read(raw_data.data(), startOffset, zsize_t(inputSize));
  startOffset.v += inputSize;
  availableSize -= inputSize;
  stream.next_in = (unsigned char*)raw_data.data();
  stream.avail_in = inputSize;
  stream.next_out = (unsigned char*) ret_data;
  stream.avail_out = _dest_size.v;
  auto errcode = OTHER;
  do {
    errcode = INFO::stream_run(&stream);
    if (errcode == BUF_ERROR) {
      if (stream.avail_in == 0 && stream.avail_out != 0)  {
        // End of input stream.
        // compressor hasn't recognize the end of the input stream but there is
        // no more input.
        // So, we must fetch a new chunk of input data.
        if (availableSize) {
          inputSize = std::min(availableSize, CHUNCK_SIZE);
          reader->read(raw_data.data(), startOffset, zsize_t(inputSize));
          startOffset.v += inputSize;
          availableSize -= inputSize;
          stream.next_in = (unsigned char*) raw_data.data();
          stream.avail_in = inputSize;
          continue;
        }
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
    if (errcode != STREAM_END && errcode != OK) {
      throw ZimFileFormatError(std::string("Invalid ") + INFO::name
                               + std::string(" stream for cluster."));
    }
  } while (errcode != STREAM_END);
  dest_size->v = stream.total_out;
  INFO::stream_end(&stream);
  return ret_data;
}

std::shared_ptr<const Buffer> Reader::get_clusterBuffer(offset_t offset, CompressionType comp) const
{
  zsize_t uncompressed_size(0);
  char* uncompressed_data = nullptr;
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
    default:
      throw std::logic_error("compressions should not be something else than zimcompLzma or zimComZip.");
  }
  return std::shared_ptr<const Buffer>(new MemoryBuffer<true>(uncompressed_data, uncompressed_size));
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
