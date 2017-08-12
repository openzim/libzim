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
#include <zim/file_reader.h>
#include <zim/file_compound.h>
#include <zim/buffer.h>
#include "config.h"
#include "envvalue.h"
#include <cstring>
#include <cassert>
#include <fstream>
#include <iostream>
#include <lzma.h>
#include <zlib.h>


namespace zim {


FileReader::FileReader(std::shared_ptr<FileCompound> source)
  : FileReader(source, 0, source->fsize()) {}

FileReader::FileReader(std::shared_ptr<FileCompound> source, std::size_t offset)
  : FileReader(source, offset, source->fsize()-offset) {}

FileReader::FileReader(std::shared_ptr<FileCompound> source, std::size_t offset, std::size_t size)
  : source(source),
    _offset(offset),
    _size(size)
{
  assert(offset<source->fsize());
  assert((offset+size)<=source->fsize());
}

char FileReader::read(std::size_t offset) {
  assert(offset < _size);
  offset += _offset;
  auto part_pair = source->lower_bound(offset);
  std::fstream stream(part_pair->second->filename());
  std::size_t local_offset = offset-part_pair->first.min;
  assert(local_offset<=part_pair->first.max);
  stream.seekg(local_offset);
  char ret;
  stream.read(&ret, 1);
  return ret;
}


void FileReader::read(char* dest, std::size_t offset, std::size_t size) {
  assert(offset < _size);
  assert(offset+size <= _size);
  if (! size ) {
    return;
  }
  offset += _offset;
  auto search_range = Range(offset, offset+size);
  auto found_range = source->equal_range(search_range);
  for(auto current = found_range.first; current!=found_range.second; current++){
    FilePart* part = current->second;
    Range partRange = current->first;
    std::size_t local_offset = offset-partRange.min;
    assert(size>0);
    size_t size_to_get = std::min(size, part->size()-local_offset);
    std::fstream stream(part->filename());
    stream.seekg(local_offset);
    stream.read(dest, size_to_get);
    dest += size_to_get;
    size -= size_to_get;
    offset += size_to_get;
  }
  assert(size==0);
}


std::shared_ptr<Buffer> FileReader::get_buffer(std::size_t offset, std::size_t size) {
  assert(size <= _size);
#if !defined(_WIN32)
  auto search_range = Range(_offset+offset, _offset+offset+size);
  auto found_range = source->equal_range(search_range);
  auto first_part_containing_it = found_range.first;
  if (++first_part_containing_it == found_range.second) {
    // The range is in only one part
    auto range = found_range.first->first;
    auto part = found_range.first->second;
    auto local_offset = offset + _offset - range.min;
    assert(size<=part->size());
    return std::unique_ptr<Buffer>(new MMapBuffer(part->filename(), local_offset, size));
  } else
#endif
  {
    // The range is several part, or we are on Windows.
    // We will have to do some memory copies :/
    // [TODO] Use Windows equivalent for mmap.
    char* p = new char[size];
    auto ret_buffer = std::unique_ptr<Buffer>(new MemoryBuffer<true>(p, size));
    read(p, offset, size);
    return std::move(ret_buffer);
  }
}

char* lzma_uncompress(const char* raw_data, size_t raw_size, size_t* dest_size) {
  // We don't know what will be the result size.
  // Let's assume it will be something like the minChunkSize used at creation
  size_t _dest_size = 1024*1024;
  char* ret_data = new char[_dest_size];
  lzma_stream stream = LZMA_STREAM_INIT;
  unsigned memsize = envMemSize("ZIM_LZMA_MEMORY_SIZE", LZMA_MEMORY_SIZE * 1024 * 1024);
  // TODO check error
  auto errcode = lzma_stream_decoder(&stream, memsize, 0);
  if (errcode != LZMA_OK) {
    //throw UnlzmaError(errcode);
    throw(6);
  }

  stream.next_in = (const unsigned char*)raw_data;
  stream.avail_in = raw_size;
  stream.next_out = (unsigned char*) ret_data;
  stream.avail_out = _dest_size;
  do {
    errcode = lzma_code(&stream, LZMA_FINISH);
    if (errcode == LZMA_BUF_ERROR) {
      //Not enought output size
      _dest_size *= 2;
      char * new_ret_data = new char[_dest_size];
      memcpy(new_ret_data, ret_data, stream.total_out);
      stream.next_out = (unsigned char*)(new_ret_data + stream.total_out);
      stream.avail_out = _dest_size - stream.total_out;
      delete [] ret_data;
      ret_data = new_ret_data;
      continue;
    }
    if (errcode != LZMA_STREAM_END && errcode != LZMA_OK) {
      throw(5);
    }
  } while (errcode != LZMA_STREAM_END);
  *dest_size = stream.total_out;
  return ret_data;
}

char* zip_uncompress(const char* raw_data, size_t raw_size, size_t* dest_size) {
  size_t _dest_size = 1024*1024;
  char* ret_data = new char[_dest_size];

  z_stream stream;
  memset(&stream, 0, sizeof(stream));

  stream.next_in = (unsigned char*) raw_data;
  stream.avail_in = raw_size;
  stream.next_out = (unsigned char*) ret_data;
  stream.avail_out = _dest_size;
  auto errcode = ::inflateInit(&stream);
  if (errcode != Z_OK) {
    throw(6);
  }
  do {

    errcode = ::inflate(&stream, Z_FINISH);
    if (errcode == Z_BUF_ERROR ) {
      //Not enought output size
      _dest_size *= 2;
      char * new_ret_data = new char[_dest_size];
      memcpy(new_ret_data, ret_data, stream.total_out);
      stream.next_out = (unsigned char*)(new_ret_data + stream.total_out);
      stream.avail_out = _dest_size - stream.total_out;
      delete [] ret_data;
      ret_data = new_ret_data;
      continue;
    }
    if (errcode != Z_STREAM_END && errcode != Z_OK) {
      throw(5);
    }
  } while ( errcode != Z_STREAM_END );
  *dest_size = stream.total_out;
  ::inflateEnd(&stream);
  return ret_data;
}

std::shared_ptr<Buffer> Reader::get_clusterBuffer(std::size_t offset, std::size_t size, CompressionType comp)
{
  auto raw_buffer = get_buffer(offset, size);
  size_t uncompressed_size;
  char* uncompressed_data = nullptr;
  switch (comp) {
    case zimcompLzma:
      uncompressed_data = lzma_uncompress(raw_buffer->data(), size, &uncompressed_size);
      break;
    case zimcompZip:
      uncompressed_data = zip_uncompress(raw_buffer->data(), size, &uncompressed_size);
      break;
    default:
      throw(5);
  }
  return std::shared_ptr<Buffer>(new MemoryBuffer<true>(uncompressed_data, uncompressed_size));
}

std::unique_ptr<Reader> Reader::sub_clusterReader(std::size_t offset, std::size_t size, CompressionType* comp) {
  *comp = static_cast<CompressionType>(read(offset));
  switch (*comp) {
    case zimcompDefault:
    case zimcompNone:
      // No compression, just a sub_reader
      return sub_reader(offset+1, size-1);
      break;
    case zimcompLzma:
    case zimcompZip:
      {
        auto buffer = get_clusterBuffer(offset+1, size-1, *comp);
        return std::unique_ptr<Reader>(new BufferReader(buffer));
      }
      break;
    default:
      throw(5);

  }
}

std::unique_ptr<Reader> FileReader::sub_reader(std::size_t offset, std::size_t size)
{
  assert(size<=_size);
  return std::unique_ptr<Reader>(new FileReader(source, _offset+offset, size));
}


//BufferReader::BufferReader(std::shared_ptr<Buffer> source)
//  : source(source) {}

std::shared_ptr<Buffer> BufferReader::get_buffer(std::size_t offset, std::size_t size)
{
  return source->sub_buffer(offset, size);
}

std::unique_ptr<Reader> BufferReader::sub_reader(std::size_t offset, std::size_t size)
{
  auto source_addr = source->data(0);
  auto sub_buff = get_buffer(offset, size);
  auto buff_addr = sub_buff->data(0);
  std::unique_ptr<Reader> sub_read(new BufferReader(sub_buff));
  return sub_read;
}

std::size_t BufferReader::size() const
{
  return source->size();
}

std::size_t BufferReader::offset() const
{
  return (std::size_t)(static_cast<const void*>(source->data(0)));
}


void BufferReader::read(char* dest, std::size_t offset, std::size_t size) {
  assert(offset < source->size());
  assert(offset+size <= source->size());
  if (! size ) {
    return;
  }
  memcpy(dest, source->data(offset), size);
}


char BufferReader::read(std::size_t offset) {
  assert(offset < source->size());
  char dest;
  dest = *source->data(offset);
  return dest;
}


} // zim
