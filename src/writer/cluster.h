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

#ifndef ZIM_WRITER_CLUSTER_H_
#define ZIM_WRITER_CLUSTER_H_

#include <zim/zim.h>
#include <zim/blob.h>
#include <iostream>
#include <vector>
#include <functional>
#include <atomic>

#include <zim/writer/item.h>
#include "../zim_types.h"

namespace zim {

namespace writer {

using writer_t = std::function<void(const Blob& data)>;
class ContentProvider;

class Cluster {
  typedef std::vector<offset_t> Offsets;
  typedef std::vector<std::unique_ptr<ContentProvider>> ClusterProviders;


  public:
    Cluster(CompressionType compression);
    virtual ~Cluster();

    void setCompression(CompressionType c) { compression = c; }
    CompressionType getCompression() const { return compression; }

    void addContent(std::unique_ptr<ContentProvider> provider);
    void addContent(const std::string& data);

    blob_index_t count() const  { return blob_index_t(m_count); }
    zsize_t size() const;
    offset_t getOffset() const { return offset; }
    void setOffset(offset_t o) { offset = o; }
    bool is_extended() const { return isExtended; }
    void clear_data();
    void close();
    bool isClosed() const;

    void setClusterIndex(cluster_index_t idx) { index = idx; }
    cluster_index_t getClusterIndex() const { return index; }

    zsize_t getBlobSize(blob_index_t n) const
    { return zsize_t(blobOffsets[blob_index_type(n)+1].v - blobOffsets[blob_index_type(n)].v); }

    offset_t getBlobOffset(blob_index_t n) const { return blobOffsets[n.v]; }
    offset_t getDataOffset() const {
      return offset_t((count().v + 1) * (isExtended?sizeof(uint64_t):sizeof(uint32_t)));
    }

    void write(int out_fd) const;

  protected:
    CompressionType compression;
    cluster_index_t index;
    bool isExtended;
    Offsets blobOffsets;
    offset_t offset;
    zsize_t _size;
    ClusterProviders m_providers;
    mutable Blob compressed_data;
    std::string tmp_filename;
    std::atomic<bool> closed { false };
    blob_index_type m_count { 0 };

  private:
    void write_content(writer_t writer) const;
    template<typename OFFSET_TYPE>
    void write_offsets(writer_t writer) const;
    void write_data(writer_t writer) const;
    void compress();
    template<typename COMP_INFO>
    void _compress();
    void clear_raw_data();
    void clear_compressed_data();
};

};

};


#endif //ZIM_WRITER_CLUSTER_H_
