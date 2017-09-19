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

#ifndef ZIM_CLUSTER_H
#define ZIM_CLUSTER_H

#include <zim/zim.h>
#include "buffer.h"
#include <iosfwd>
#include <vector>
#include <memory>

namespace zim
{
  class Blob;
  class Reader;

  class Cluster : public std::enable_shared_from_this<Cluster> {
      typedef std::vector<size_type> Offsets;

      const CompressionType compression;
      Offsets offsets;
      std::shared_ptr<const Reader> reader;
      offset_type startOffset;

      offset_type read_header();

    public:
      Cluster(std::shared_ptr<const Reader> reader, CompressionType comp);
      CompressionType getCompression() const   { return compression; }
      bool isCompressed() const                { return compression == zimcompZip || compression == zimcompBzip2 || compression == zimcompLzma; }

      size_type count() const               { return offsets.size() - 1; }
      size_type size() const;

      const char* getBlobPtr(unsigned n) const;
      size_type getBlobSize(unsigned n) const  { return offsets[n+1] - offsets[n]; }
      offset_type getBlobOffset(size_type n) const { return startOffset + offsets[n]; }
      Blob getBlob(size_type n) const;
      void clear();

      void init_from_buffer(Buffer& buffer);
  };

}

#endif // ZIM_CLUSTER_H
