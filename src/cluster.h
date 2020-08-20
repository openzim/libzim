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
#include "zim_types.h"
#include "file_reader.h"
#include <iosfwd>
#include <vector>
#include <memory>

#include "zim_types.h"

namespace zim
{
  class Blob;
  class Reader;

  class Cluster : public std::enable_shared_from_this<Cluster> {
      typedef std::vector<offset_t> Offsets;

      const CompressionType compression;
      const bool isExtended;
      std::shared_ptr<const Reader> reader;

      // offset of the first blob of this cluster relative to the beginning
      // of the (uncompressed) cluster data
      offset_t startOffset;

      // offsets of the blob boundaries relative to the start of the first
      // blob in this cluster. For a cluster with N blobs, this collection
      // contains N+1 entries - the start of the first blob (which is always
      // 0) and the end of the last blob are also included.
      Offsets offsets;

      template<typename OFFSET_TYPE>
      offset_t read_header();

    public:
      Cluster(std::shared_ptr<const Reader> reader, CompressionType comp, bool isExtended);
      CompressionType getCompression() const   { return compression; }
      bool isCompressed() const                { return compression != zimcompDefault && compression != zimcompNone; }

      blob_index_t count() const               { return blob_index_t(offsets.size() - 1); }

      zsize_t getBlobSize(blob_index_t n) const  { return zsize_t(offsets[blob_index_type(n)+1].v
                                                                - offsets[blob_index_type(n)].v); }
      offset_t getBlobOffset(blob_index_t n) const { return startOffset + offsets[blob_index_type(n)]; }
      Blob getBlob(blob_index_t n) const;
      Blob getBlob(blob_index_t n, offset_t offset, zsize_t size) const;

      static zsize_t read_size(const Reader* reader, bool isExtended, offset_t offset);
  };

}

#endif // ZIM_CLUSTER_H
