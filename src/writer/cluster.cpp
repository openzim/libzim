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

namespace zim {
namespace writer {

Cluster::Cluster(CompressionType compression)
  : compression(compression),
    isExtended(false),
    _size(0)
{
  offsets.push_back(offset_t(0));
}

void Cluster::clear() {
  offsets.clear();
  _data.clear();
  offsets.push_back(offset_t(0));
  isExtended = false;
  _size = zsize_t(0);
}

zsize_t Cluster::size() const
{
  if (isExtended) {
    return zsize_t(offsets.size() * sizeof(uint64_t)) + _size;
  } else {
    return zsize_t(offsets.size() * sizeof(uint32_t)) + _size;
  }
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

void Cluster::write(std::ostream& out) const
{
  if (isExtended) {
    write_offsets<uint64_t>(out);
  } else {
    write_offsets<uint32_t>(out);
  }
  write_data(out);
}

std::ostream& operator<< (std::ostream& out, const Cluster& cluster)
{
  log_trace("write cluster");

  char clusterInfo = 0;
  if (cluster.isExtended) {
    clusterInfo = 0x10;
  }
  clusterInfo += cluster.getCompression();
  out.put(clusterInfo);

  switch(cluster.getCompression())
  {
    case zim::zimcompDefault:
    case zim::zimcompNone:
      cluster.write(out);
      break;

    case zim::zimcompZip:
      {
#if defined(ENABLE_ZLIB)
        log_debug("compress data (zlib)");
        zim::writer::DeflateStream os(out);
        os.exceptions(std::ios::failbit | std::ios::badbit);
        cluster.write(os);
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
        cluster.write(os);
        os.end();
        break;
      }

    default:
      std::ostringstream msg;
      msg << "invalid compression flag " << cluster.getCompression();
      log_error(msg.str());
      throw std::runtime_error(msg.str());
  }

  return out;
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

  _data.emplace_back(DataType::plain, std::string(data, size.v));
}

void Cluster::write_data(std::ostream& out) const
{
  for (auto& data: _data)
  {
    ASSERT(data.value.empty(), ==, false);
    if (data.type == DataType::plain) {
      out << data.value;
    } else {
      std::ifstream stream(data.value);
      out << stream.rdbuf();
    }
  }
}

} // writer
} // zim
