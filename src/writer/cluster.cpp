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

#include "cluster.h"
#include "../log.h"
#include "../endian_tools.h"
#include "../debug.h"
#include "../compression.h"
#include "../fs.h"

#include <sstream>
#include <fstream>

#ifdef _WIN32
#define SEPARATOR "\\"
#else
#define SEPARATOR "/"
#endif

namespace zim {
namespace writer {

Cluster::Cluster(CompressionType compression)
  : compression(compression),
    isExtended(false),
    _size(0)
{
  blobOffsets.push_back(offset_t(0));
  pthread_mutex_init(&m_closedMutex,NULL);
}

void Cluster::clear() {
  Offsets().swap(blobOffsets);
  ClusterData().swap(_data);
}

void Cluster::close() {
  pthread_mutex_lock(&m_closedMutex);
  closed = true;
  pthread_mutex_unlock(&m_closedMutex);
}

bool Cluster::isClosed() const{
  bool v;
  pthread_mutex_lock(&m_closedMutex);
  v = closed;
  pthread_mutex_unlock(&m_closedMutex);
  return v;
}

zsize_t Cluster::size() const
{
  if (isClosed()) {
    throw std::runtime_error("oups");
  }
  if (isExtended) {
    return zsize_t(blobOffsets.size() * sizeof(uint64_t)) + _size;
  } else {
    return zsize_t(blobOffsets.size() * sizeof(uint32_t)) + _size;
  }
}

template<typename OFFSET_TYPE>
void Cluster::write_offsets(writer_t writer) const
{
  size_type delta = blobOffsets.size() * sizeof(OFFSET_TYPE);
  char out_buf[sizeof(OFFSET_TYPE)];
  for (auto offset : blobOffsets)
  {
    offset.v += delta;
    toLittleEndian(static_cast<OFFSET_TYPE>(offset.v), out_buf);
    writer(Blob(out_buf, sizeof(OFFSET_TYPE)));
  }
}

void Cluster::write_final(int out_fd) const
{
  {
    int fd = open(tmp_filename.c_str(), O_RDONLY);
    if (fd == -1) {
      throw std::runtime_error(std::string("cannot open ") + tmp_filename);
    }
    char* buffer = new char[1024*1024];
    while (true) {
      auto r = read(fd, (char*)buffer, 1024*1024);
      if (! r)
        break;
      ::write(out_fd, buffer, r);
    }
    delete[] buffer;
    ::close(fd);
  }
  DEFAULTFS::removeFile(tmp_filename);
}

void Cluster::dump_tmp(const std::string& directoryPath)
{
  std::ostringstream ss;
  ss << directoryPath << SEPARATOR << "cluster_" << index << ".clt";
  tmp_filename = ss.str();
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  int tmp_fd = open(tmp_filename.c_str(), O_WRONLY|O_CREAT|O_TRUNC, mode);
  if (tmp_fd == -1) {
    throw std::runtime_error(
      std::string("failed to create temporary cluster file ")
    + tmp_filename);
  }
  dump(tmp_fd);
  ::close(tmp_fd);
  clear();
}

void Cluster::write(writer_t writer) const
{
  if (isExtended) {
    write_offsets<uint64_t>(writer);
  } else {
    write_offsets<uint32_t>(writer);
  }
  write_data(writer);
}

void Cluster::compress()
{
  auto comp = getCompression();
  switch(comp) {
    case zim::zimcompBzip2:
#if !defined(ENABLE_ZLIB)
    case zim::zimcompZip:
#endif
      {
        throw std::runtime_error("Compression method not enabled in this library");
        break;
      }

    case zim::zimcompLzma:
#if defined(ENABLE_ZLIB)
    case zim::zimcompZip:
#endif
      {
        if (comp == zim::zimcompLzma) {
          _compress<LZMA_INFO>();
          break;
        };
#if defined(ENABLE_ZLIB)
        _compress<ZIP_INFO>();
        break;
#endif
      }
    default:
      throw std::runtime_error("We cannot compress an uncompressed cluster");
  };
}

template<typename COMP_TYPE>
void Cluster::_compress()
{
  Compressor<COMP_TYPE> runner;
  bool first = true;
  auto writer = [&](const Blob& data) -> void {
    if (first) {
      runner.init((char*)data.data());
      first = false;
    }
    runner.feed(data.data(), data.size());
  };
  write(writer);
  zsize_t size;
  auto comp = runner.get_data(&size);
  compressed_data = Blob(comp, size.v);
}

void Cluster::dump(int tmp_fd)
{
  // write clusterInfo
  char clusterInfo = 0;
  if (isExtended) {
    clusterInfo = 0x10;
  }
  clusterInfo += getCompression();
  ::write(tmp_fd, &clusterInfo, 1);

  // Open a comprestion stream if needed
  switch(getCompression())
  {
    case zim::zimcompDefault:
    case zim::zimcompNone:
    {
      auto writer = [=](const Blob& data) -> void {
        // Ideally we would simply have to do :
        // ::write(tmp_fd, data.c_str(), data.size());
        // However, the data can be pretty big (> 4Gb), especially with test,
        // And ::write fails to write data > 4Gb. So we have to chunck the write.
        size_type to_write = data.size();
        const char* src = data.data();
        while (to_write) {
         size_type chunk_size = to_write > 4096 ? 4096 : to_write;
         auto ret = ::write(tmp_fd, src, chunk_size);
         src += ret;
         to_write -= ret;
        }
      };
      write(writer);
      break;
    }

    case zim::zimcompZip:
    case zim::zimcompBzip2:
    case zim::zimcompLzma:
      {
        log_debug("compress data");
        compress();
        ::write(tmp_fd, compressed_data.data(), compressed_data.size());
        delete [] compressed_data.data();
        break;
      }

    default:
      std::ostringstream msg;
      msg << "invalid compression flag " << getCompression();
      log_error(msg.str());
      throw std::runtime_error(msg.str());
  }
}

void Cluster::addArticle(const zim::writer::Article* article)
{
  auto filename = article->getFilename();
  auto size = article->getSize();
  _size += size;
  blobOffsets.push_back(offset_t(_size.v));
  isExtended |= (size>UINT32_MAX);
  if (size == 0)
    return;

  if (filename.empty()) {
    _data.emplace_back(DataType::plain, article->getData());
  }
  else {
    _data.emplace_back(DataType::file, filename);
  }
}

void Cluster::addData(const char* data, zsize_t size)
{
  _size += size;
  blobOffsets.push_back(offset_t(_size.v));
  isExtended |= (size.v>UINT32_MAX);
  if (size.v == 0)
    return;

  _data.emplace_back(DataType::plain, data, size.v);
}

void Cluster::write_data(writer_t writer) const
{
  for (auto& data: _data)
  {
    ASSERT(data.value.empty(), ==, false);
    if (data.type == DataType::plain) {
      writer(Blob(data.value.c_str(), data.value.size()));
    } else {
      int fd = open(data.value.c_str(), O_RDONLY);
      if (fd == -1) {
        throw std::runtime_error(std::string("cannot open ") + data.value);
      }
      char* buffer = new char[1024*1024];
      while (true) {
        auto r = read(fd, buffer, 1024*1024);
        if (!r)
          break;
        writer(Blob(buffer, r));
      }
      delete [] buffer;
      ::close(fd);
    }
  }
}

} // writer
} // zim
