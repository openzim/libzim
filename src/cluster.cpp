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

#if defined(ENABLE_ZLIB)
#include "inflatestream.h"
#endif

#if defined(ENABLE_LZMA)
#include "unlzmastream.h"
#endif

log_define("zim.cluster")

#define log_debug1(e)

namespace zim
{

  Cluster::Cluster()
    : compression(zimcompNone),
      startOffset(0),
      lazy_read_stream(NULL)
  {
    offsets.push_back(0);
  }

  /* This return the number of char read */
  offset_type Cluster::read_header(std::istream& in)
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

  void Cluster::read_content(std::istream& in)
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

  void Cluster::finalise_read() {
    if ( !lazy_read_stream )
    {
        std::cerr << "lazy_read null" << std::endl;
        return;
    }
    lazy_read_stream->seekg(startOffset);
    read_content(*lazy_read_stream);
    lazy_read_stream = NULL;
  }

  Blob Cluster::getBlob(size_type n) const
  {
    size_type s = size();
    return s > 0 ? Blob(shared_from_this(), getBlobPtr(n), getBlobSize(n))
                 : Blob();
  }

  void Cluster::clear()
  {
    offsets.clear();
    _data.clear();
    offsets.push_back(0);
  }

  void Cluster::init_from_stream(ifstream& in, offset_type offset)
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
#if defined(ENABLE_ZLIB)
          log_debug("uncompress data (zlib)");
          zim::InflateStream is(in);
          is.exceptions(std::ios::failbit | std::ios::badbit);
          read_header(is);
          read_content(is);
#else
          throw std::runtime_error("zlib not enabled in this library");
#endif
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

}
