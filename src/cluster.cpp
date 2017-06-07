/*
 * Copyright (C) 2009 Tommi Maekitalo
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

#include <zim/cluster.h>
#include <zim/blob.h>
#include "endian_tools.h"
#include <zim/error.h>
#include <stdlib.h>
#include <sstream>

#include "log.h"

#include "config.h"

#if defined(ENABLE_LZMA)
#include "lzmastream.h"
#include "unlzmastream.h"
#endif

log_define("zim.cluster")

#define log_debug1(e)

namespace zim
{
  Cluster::Cluster()
    : impl(0)
    { }

  ClusterImpl* Cluster::getImpl()
  {
    if (impl.getPointer() == 0)
      impl = new ClusterImpl();
    return impl;
  }

  ClusterImpl::ClusterImpl()
    : compression(zimcompNone),
      startOffset(0),
      lazy_read_stream(NULL)
  {
    offsets.push_back(0);
  }

  /* This return the number of char read */
  offset_type ClusterImpl::read_header(std::istream& in)
  {
    log_debug1("read_header");
    // read first offset, which specifies, how many offsets we need to read
    size_type offset;
    in.read(reinterpret_cast<char*>(&offset), sizeof(offset));
    if (in.fail())
    {
      std::cerr << "fail at read offset" << std::endl;
      throw ZimFileFormatError("fail at read first offset");
    }

    offset = fromLittleEndian(&offset);

    size_type n = offset / 4;
    size_type a = offset;

    log_debug1("first offset is " << offset << " n=" << n << " a=" << a);

    // read offsets
    offsets.clear();
    offsets.reserve(n);
    offsets.push_back(0);
    while (--n)
    {
      in.read(reinterpret_cast<char*>(&offset), sizeof(offset));
      if (in.fail())
      {
        log_debug("fail at " << n);
        throw ZimFileFormatError("fail at read offset");
      }
      offset = fromLittleEndian(&offset);
      log_debug1("offset=" << offset << '(' << offset-a << ')');
      offsets.push_back(offset - a);
    }
    return a;
  }

  void ClusterImpl::read_content(std::istream& in)
  {
    log_debug1("read_content");
    _data.clear();
    // last offset points past the end of the cluster, so we know now, how may bytes to read
    if (offsets.size() > 1)
    {
      size_type n = offsets.back() - offsets.front();
      if (n > 0)
      {
        _data.resize(n);
        log_debug("read " << n << " bytes of data");
        in.read(&(_data[0]), n);
      }
      else
        log_warn("read empty cluster");
    }
  }

  void ClusterImpl::finalise_read() {
    if ( !lazy_read_stream )
    {
        std::cerr << "lazy_read null" << std::endl;
        return;
    }
    lazy_read_stream->seekg(startOffset);
    read_content(*lazy_read_stream);
    lazy_read_stream = NULL;
  }

  void ClusterImpl::write(std::ostream& out) const
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

  void ClusterImpl::addBlob(const Blob& blob)
  {
    log_debug1("addBlob(ptr, " << blob.size() << ')');
    _data.insert(_data.end(), blob.data(), blob.end());
    offsets.push_back(_data.size());
  }

  Blob ClusterImpl::getBlob(size_type n) const
  {
    size_type s = getSize();
    return s > 0 ? Blob(const_cast<ClusterImpl*>(this), getData(n), getSize(n))
                 : Blob();
  }

  void ClusterImpl::clear()
  {
    offsets.clear();
    _data.clear();
    offsets.push_back(0);
  }

  void ClusterImpl::addBlob(const char* data, unsigned size)
  {
    addBlob(Blob(data, size));
  }

  Blob Cluster::getBlob(size_type n) const
  {
    return impl->getBlob(n);
  }

  void Cluster::init_from_stream(ifstream& in, offset_type offset)
  {
    getImpl()->init_from_stream(in, offset);
  }

  void ClusterImpl::init_from_stream(ifstream& in, offset_type offset)
  {
    log_trace("init_from_stream");
    in.seekg(offset);

    clear();

    char c;
    in.get(c);
    setCompression(static_cast<CompressionType>(c));

    switch (static_cast<CompressionType>(c))
    {
      case zimcompDefault:
      case zimcompNone:
        startOffset = read_header(in);
        startOffset += sizeof(char) + offset;
        set_lazy_read(&in);
        break;

      case zimcompZip:
        {
          throw std::runtime_error("zlib not enabled in this library");
          break;
        }

      case zimcompBzip2:
        {
          throw std::runtime_error("bzip2 not enabled in this library");
          break;
        }

      case zimcompLzma:
        {
#if defined(ENABLE_LZMA)
          log_debug("uncompress data (lzma)");
          zim::UnlzmaStream is(in);
          is.exceptions(std::ios::failbit | std::ios::badbit);
          read_header(is);
          read_content(is);
#else
          throw std::runtime_error("lzma not enabled in this library");
#endif
          break;
        }

      default:
        log_error("invalid compression flag " << c);
        in.setstate(std::ios::failbit);
        break;
    }
  }

  std::ostream& operator<< (std::ostream& out, const ClusterImpl& clusterImpl)
  {
    log_trace("write cluster");

    out.put(static_cast<char>(clusterImpl.getCompression()));

    switch(clusterImpl.getCompression())
    {
      case zimcompDefault:
      case zimcompNone:
        clusterImpl.write(out);
        break;

      case zimcompZip:
        {
          throw std::runtime_error("zlib not enabled in this library");
          break;
        }

      case zimcompBzip2:
        {
          throw std::runtime_error("bzip2 not enabled in this library");
          break;
        }

      case zimcompLzma:
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
          zim::LzmaStream os(out, lzmaPreset);
          os.exceptions(std::ios::failbit | std::ios::badbit);
          clusterImpl.write(os);
          os.end();
#else
          throw std::runtime_error("lzma not enabled in this library");
#endif
          break;
        }

      default:
        std::ostringstream msg;
        msg << "invalid compression flag " << clusterImpl.getCompression();
        log_error(msg.str());
        throw std::runtime_error(msg.str());
    }

    return out;
  }

  std::ostream& operator<< (std::ostream& out, const Cluster& cluster)
  {
    return out << *cluster.impl;
  }
}
