/*
 * Copyright (C) 2017-2021 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2021 Veloman Yunkan
 * Copyright (C) 2020 Emmanuel Engelhart <kelson@kiwix.org>
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

#include <zim/writer/contentProvider.h>
#include <zim/tools.h>
#include <fstream>

#include <fcntl.h>
#include <stdexcept>

#ifdef _WIN32
# include <io.h>
#else
# include <unistd.h>
# define _write(fd, addr, size) ::write((fd), (addr), (size))
#endif

const zim::size_type MAX_WRITE_SIZE(4UL*1024*1024*1024-1);

namespace zim {
namespace writer {

Cluster::Cluster(Compression compression)
  : compression(compression),
    isExtended(false),
    _size(0)
{
  blobOffsets.push_back(offset_t(0));
}

Cluster::~Cluster() {
  if (compressed_data.data()) {
    delete[] compressed_data.data();
  }
}

void Cluster::clear_data() {
  clear_raw_data();
  clear_compressed_data();
}

void Cluster::clear_raw_data() {
  Offsets().swap(blobOffsets);
  ClusterProviders().swap(m_providers);
}

void Cluster::clear_compressed_data() {
  if (compressed_data.data()) {
    delete[] compressed_data.data();
    compressed_data = Blob();
  }
}

void Cluster::close() {
  if (getCompression() != Compression::None) {
    // We must compress the content in a buffer.
    compress();
    clear_raw_data();
  }
  closed = true;
}

bool Cluster::isClosed() const{
  return closed;
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

void Cluster::write_content(writer_t writer) const
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
    case Compression::Zstd:
      {
        _compress<ZSTD_INFO>();
        break;
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
  write_content(writer);
  zsize_t size;
  auto comp = runner.get_data(&size);
  compressed_data = Blob(comp.release(), size.v);
}

void Cluster::write(int out_fd) const
{
  // write clusterInfo
  char clusterInfo = 0;
  if (isExtended) {
    clusterInfo = 0x10;
  }
  clusterInfo += static_cast<uint8_t>(getCompression());
  if (_write(out_fd, &clusterInfo, 1) == -1) {
    throw std::runtime_error("Error writing");
  }

  // Open a comprestion stream if needed
  switch(getCompression())
  {
    case Compression::None:
    {
      auto writer = [=](const Blob& data) -> void {
        // Ideally we would simply have to do :
        // ::write(tmp_fd, data.c_str(), data.size());
        // However, the data can be pretty big (> 4Gb), especially with test,
        // And ::write fails to write data > 4Gb. So we have to chunck the write.
        size_type to_write = data.size();
        const char* src = data.data();
        while (to_write) {
         size_type chunk_size = std::min(MAX_WRITE_SIZE, to_write);
         auto ret = _write(out_fd, src, chunk_size);
         src += ret;
         to_write -= ret;
        }
      };
      write_content(writer);
      break;
    }

    case Compression::Zstd:
      {
        log_debug("compress data");
        if (_write(out_fd, compressed_data.data(), compressed_data.size()) == -1) {
          throw std::runtime_error("Error writing");
        }
        break;
      }

    default:
      Formatter fmt_msg;
      fmt_msg << "invalid compression flag " << static_cast<uint8_t>(getCompression());
      log_error(fmt_msg);
      throw std::runtime_error(fmt_msg);
  }
}


void Cluster::addContent(std::unique_ptr<ContentProvider> provider)
{
  auto size = provider->getSize();
  _size += size;
  blobOffsets.push_back(offset_t(_size.v));
  m_count++;
  isExtended |= (_size.v>UINT32_MAX);
  if (size == 0)
    return;

  m_providers.push_back(std::move(provider));
}

void Cluster::addContent(const std::string& data)
{
  auto contentProvider = std::unique_ptr<ContentProvider>(new StringProvider(data));
  addContent(std::move(contentProvider));
}

void Cluster::write_data(writer_t writer) const
{
  for (auto& provider: m_providers)
  {
    ASSERT(provider->getSize(), !=, 0U);
    zim::size_type size = 0;
    while(true) {
      auto blob = provider->feed();
      if(blob.size() == 0) {
        break;
      }
      size += blob.size();
      writer(blob);
    }
    if (size != provider->getSize())
      throw IncoherentImplementationError(
          Formatter()
          << "Declared provider's size (" << provider->getSize() << ")"
          << " is not equal to total size returned by feed() calls (" << size
          << ").");
  }
}

} // writer
} // zim
