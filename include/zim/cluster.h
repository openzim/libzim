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
#include <zim/fstream.h>
#include <iosfwd>
#include <vector>
#include <memory>

namespace zim
{
  class Blob;

  class Cluster : public std::enable_shared_from_this<Cluster>
  {
      typedef std::vector<size_type> Offsets;
      typedef std::vector<char> Data;

      CompressionType compression;
      Offsets offsets;
      Data _data;
      offset_type startOffset;

      ifstream* lazy_read_stream;

      offset_type read_header(std::istream& in);
      void read_content(std::istream& in);

      void set_lazy_read(ifstream* in) {
        lazy_read_stream = in;
      }

      bool is_fully_initialised() const { return lazy_read_stream == 0; }
      void finalise_read();
      const Data& data() const {
        if ( !is_fully_initialised() )
        {
           const_cast<Cluster*>(this)->finalise_read();
        }
        return _data;
      }

    public:
      Cluster();
      void setCompression(CompressionType c)   { compression = c; }
      CompressionType getCompression() const   { return compression; }
      bool isCompressed() const                { return compression == zimcompZip || compression == zimcompBzip2 || compression == zimcompLzma; }

      size_type count() const               { return offsets.size() - 1; }
      size_type size() const                { return offsets.size() * sizeof(size_type) + data().size(); }
      const char* getBlobPtr(unsigned n) const    { return &data()[ offsets[n] ]; }
      size_type getBlobSize(unsigned n) const      { return offsets[n+1] - offsets[n]; }
      offset_type getBlobOffset(size_type n) const { return startOffset + offsets[n]; }
      Blob getBlob(size_type n) const;
      void clear();

      void init_from_stream(ifstream& in, offset_type offset);
  };

}

#endif // ZIM_CLUSTER_H
