/*
 * Copyright (C) 2016-2021 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2020 Veloman Yunkan
 * Copyright (C) 2020 Miguel Rocha
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
#include <vector>
#include <memory>
#include <mutex>

#include "zim_types.h"

namespace zim
{
  class Blob;
  class Reader;
  class IStreamReader;

  class LIBZIM_API Cluster : public std::enable_shared_from_this<Cluster> {
      typedef std::vector<offset_t> BlobOffsets;
      typedef std::vector<std::unique_ptr<const Reader>> BlobReaders;

    public:
      // zim::Compression lists only compression methods supported by the
      // writer. But on the reader side we need to deal with some historical
      // compression types. Here we maintain the full list of compression
      // types.
      enum class Compression
      {
        None = 1,
        Zip,      // Support is discontinued
        Bzip2,    // Support is discontinued
        Lzma,     // Supported only by the reader
        Zstd
      };

    public:
      const Compression compression;
      const bool isExtended;

    private:
      std::unique_ptr<IStreamReader> m_reader;

      // offsets of the blob boundaries relative to the start of the cluster data
      // (*after* the first byte (clusterInfo))
      // For a cluster with N blobs, this collection contains N+1 entries.
      // The start of the first blob and the end of the last blob are included.
      BlobOffsets m_blobOffsets;

      mutable std::mutex m_readerAccessMutex;
      mutable BlobReaders m_blobReaders;


      template<typename OFFSET_TYPE>
      void read_header();
      const Reader& getReader(blob_index_t n) const;

    public:
      Cluster(std::unique_ptr<IStreamReader> reader, Compression comp, bool isExtended);
      Compression getCompression() const   { return compression; }
      bool isCompressed() const                { return compression != Compression::None; }

      blob_index_t count() const               { return blob_index_t(m_blobOffsets.size() - 1); }

      zsize_t getBlobSize(blob_index_t n) const;

      offset_t getBlobOffset(blob_index_t n) const { return offset_t(1) + m_blobOffsets[blob_index_type(n)]; }
      Blob getBlob(blob_index_t n) const;
      Blob getBlob(blob_index_t n, offset_t offset, zsize_t size) const;

      static std::shared_ptr<Cluster> read(const Reader& zimReader, offset_t clusterOffset);
  };

}

#endif // ZIM_CLUSTER_H
