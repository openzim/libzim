/*
 * Copyright (C) 2009 Tommi Maekitalo
 * Copyright (C) 2020 Veloman Yunkan
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
#include "reader.h"
#include "idatastream.h"
#include <iosfwd>
#include <vector>
#include <memory>
#include <mutex>

#include "zim_types.h"
#include "zim/error.h"

namespace zim
{

class Blob;
class Reader;

class Cluster : public std::enable_shared_from_this<Cluster> {
  public:
    const bool isExtended;

  protected:
    explicit Cluster(bool isExtended) : isExtended(isExtended) {}

  public:
    virtual ~Cluster() {}

    virtual bool isCompressed() const = 0;
    virtual CompressionType getCompression() const = 0;

    virtual blob_index_t count() const = 0;

    virtual zsize_t getBlobSize(blob_index_t n) const = 0;
    virtual offset_t getBlobOffset(blob_index_t n) const = 0;
    virtual Blob getBlob(blob_index_t n) const = 0;
    virtual Blob getBlob(blob_index_t n, offset_t offset, zsize_t size) const = 0;

    static std::shared_ptr<Cluster> read(const Reader& zimReader, offset_t clusterOffset);
};

class NonCompressedCluster : public Cluster {
    typedef std::vector<offset_t> Offsets;

  private:
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
    NonCompressedCluster(std::shared_ptr<const Reader> reader, bool isExtended);

    bool isCompressed() const override { return false; }
    CompressionType getCompression() const override { return zimcompNone; }

    blob_index_t count() const override { return blob_index_t(offsets.size() - 1); }

    zsize_t getBlobSize(blob_index_t n) const override;
    offset_t getBlobOffset(blob_index_t n) const override { return startOffset + offsets[blob_index_type(n)]; }
    Blob getBlob(blob_index_t n) const override;
    Blob getBlob(blob_index_t n, offset_t offset, zsize_t size) const override;
};

class CompressedCluster : public Cluster
{
  public: // functions
    CompressedCluster(std::shared_ptr<const Reader> reader, CompressionType comp, bool isExtended);

    bool isCompressed() const override;
    CompressionType getCompression() const override;
    blob_index_t count() const override;
    zsize_t getBlobSize(blob_index_t n) const override;
    offset_t getBlobOffset(blob_index_t n) const override;
    Blob getBlob(blob_index_t n) const override;
    Blob getBlob(blob_index_t n, offset_t offset, zsize_t size) const override;

  private: // functions
    template<typename OFFSET_TYPE>
    void readHeader(IDataStream& ds);

    void ensureBlobIsDecompressed(blob_index_t i) const;

  private: // data
    const std::unique_ptr<IDataStream> ds_;
    const CompressionType compression_;
    std::vector<size_t> blobSizes_;

    mutable std::mutex blobAccessMutex_;
    mutable std::vector<IDataStream::Blob> blobs_;
};

} // unnamed namespace

#endif // ZIM_CLUSTER_H
