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

#include <sstream>
#include <fstream>

#if defined(ENABLE_ZLIB)
#include "deflatestream.h"
#endif

#include "lzmastream.h"

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
  offsets.push_back(offset_t(0));
  pthread_mutex_init(&m_closedMutex,NULL);
}

void Cluster::clear() {
  Offsets().swap(offsets);
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
    return zsize_t(offsets.size() * sizeof(uint64_t)) + _size;
  } else {
    return zsize_t(offsets.size() * sizeof(uint32_t)) + _size;
  }
}

zsize_t Cluster::getFinalSize() const
{
  return finalSize;
}

template<typename OFFSET_TYPE>
void Cluster::write_offsets(std::ostream& out) const
{
  size_type delta = offsets.size() * sizeof(OFFSET_TYPE);
  for (auto offset : offsets)
  {
    offset.v += delta;
    char out_buf[sizeof(OFFSET_TYPE)];
    toLittleEndian(static_cast<OFFSET_TYPE>(offset.v), out_buf);
    out.write(out_buf, sizeof(OFFSET_TYPE));
  }
}

void Cluster::write_final(std::ostream& out) const
{
  std::ifstream clustersFile(tmp_filename, std::ios::binary);
  out << clustersFile.rdbuf();
  if (!out) {
    throw std::runtime_error("failed to write cluster");
  }
}

void Cluster::dump_tmp(const std::string& directoryPath)
{
  std::ostringstream ss;
  ss << directoryPath << SEPARATOR << "cluster_" << index << ".clt";
  tmp_filename = ss.str();
  std::ofstream out(tmp_filename, std::ios::binary);
  dump(out);
  if (!out) {
    throw std::runtime_error(
      std::string("failed to write temporary cluster file ")
    + tmp_filename);
  }
  finalSize = zsize_t(out.tellp());
  clear();
}

void Cluster::write(std::ostream& out) const
{
  if (isExtended) {
    write_offsets<uint64_t>(out);
  } else {
    write_offsets<uint32_t>(out);
  }
  write_data(out);
}

void Cluster::dump(std::ostream& out) const
{
  // write clusterInfo
  char clusterInfo = 0;
  if (isExtended) {
    clusterInfo = 0x10;
  }
  clusterInfo += getCompression();
  out.put(clusterInfo);

  // Open a comprestion stream if needed
  switch(getCompression())
  {
    case zim::zimcompDefault:
    case zim::zimcompNone:
      write(out);
      break;

    case zim::zimcompZip:
      {
#if defined(ENABLE_ZLIB)
        log_debug("compress data (zlib)");
        zim::writer::DeflateStream os(out);
        os.exceptions(std::ios::failbit | std::ios::badbit);
        write(os);
        os.flush();
        os.end();
#else
        throw std::runtime_error("zlib not enabled in this library");
#endif
        break;
      }

    case zim::zimcompBzip2:
      {
        throw std::runtime_error("bzip2 not enabled in this library");
        break;
      }

    case zim::zimcompLzma:
      {
        uint32_t lzmaPreset = 3 | LZMA_PRESET_EXTREME;
        /**
         * read lzma preset from environment
         * ZIM_LZMA_PRESET is a number followed optionally by a
         * suffix 'e'. The number gives the preset and the suffix tells,
         * if LZMA_PRESET_EXTREME should be set.
         * e.g.:
         *   ZIM_LZMA_LEVEL=9   => 9
         *   ZIM_LZMA_LEVEL=3e  => 3 + extreme
         */
        const char* e = ::getenv("ZIM_LZMA_LEVEL");
        if (e)
        {
          char flag = '\0';
          std::istringstream s(e);
          s >> lzmaPreset >> flag;
          if (flag == 'e')
            lzmaPreset |= LZMA_PRESET_EXTREME;
        }

        log_debug("compress data (lzma, " << std::hex << lzmaPreset << ")");
        zim::writer::LzmaStream os(out, lzmaPreset);
        os.exceptions(std::ios::failbit | std::ios::badbit);
        write(os);
        os.end();
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
  offsets.push_back(offset_t(_size.v));
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
  offsets.push_back(offset_t(_size.v));
  isExtended |= (size.v>UINT32_MAX);
  if (size.v == 0)
    return;

  _data.emplace_back(DataType::plain, data, size.v);
}

void Cluster::write_data(std::ostream& out) const
{
  for (auto& data: _data)
  {
    ASSERT(data.value.empty(), ==, false);
    if (data.type == DataType::plain) {
      out << data.value;
    } else {
      std::ifstream stream(data.value, std::ios::binary);
      if (!stream) {
         throw std::runtime_error(std::string("cannot open ") + data.value);
      }
      out << stream.rdbuf();
      if (!out) {
        throw std::runtime_error(std::string("failed to write file ") + data.value);
      }
    }
  }
}

} // writer
} // zim
