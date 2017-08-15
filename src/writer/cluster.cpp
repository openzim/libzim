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

#include <sstream>

#if defined(ENABLE_ZLIB)
#include "deflatestream.h"
#endif

#if defined(ENABLE_LZMA)
#include "lzmastream.h"
#endif

namespace zim {
namespace writer {

Cluster::Cluster()
  : compression(zimcompNone)
{
  offsets.push_back(0);
}

void Cluster::clear() {
  offsets.clear();
  _data.clear();  
  offsets.push_back(0);
}

void Cluster::addBlob(const Blob& blob)
{
  _data.insert(_data.end(), blob.data(), blob.end());
  offsets.push_back(_data.size());
}


void Cluster::addBlob(const char* data, unsigned size)
{
  addBlob(Blob(data, size));
}


void Cluster::write(std::ostream& out) const
{
  size_type a = offsets.size() * sizeof(size_type);
  for (Offsets::const_iterator it = offsets.begin(); it != offsets.end(); ++it)
  {
    size_type o = *it;
    o += a;
    o = fromLittleEndian(&o);
    out.write(reinterpret_cast<const char*>(&o), sizeof(size_type));
  }

  if (_data.size() > 0)
    out.write(&(_data[0]), _data.size());
  else
    log_warn("write empty cluster");
}

std::ostream& operator<< (std::ostream& out, const Cluster& cluster)
{
  log_trace("write cluster");

  out.put(static_cast<char>(cluster.getCompression()));

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
#if defined(ENABLE_LZMA)
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
#else
        throw std::runtime_error("lzma not enabled in this library");
#endif
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


} // writer
} // zim 
