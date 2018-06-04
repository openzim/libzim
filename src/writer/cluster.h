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
#include <pthread.h>

#include <zim/writer/article.h>
#include "../zim_types.h"

namespace zim {

namespace writer {

enum class DataType { plain, file };
struct Data {
  Data(zim::writer::DataType type, const std::string& value) :
    type(type), value(value) {}
  Data(zim::writer::DataType type, const char* data, zim::size_type size) :
    type(type), value(data, size) {}
  DataType type;
  std::string value;
};

class Cluster {
  typedef std::vector<offset_t> Offsets;
  typedef std::vector<Data> ClusterData;


  public:
    Cluster(CompressionType compression);
    virtual ~Cluster() { pthread_mutex_destroy(&m_closedMutex);}

    void setCompression(CompressionType c) { compression = c; }
    CompressionType getCompression() const { return compression; }

    void addArticle(const zim::writer::Article* article);
    void addData(const char* data, zsize_t size);

    blob_index_t count() const  { return blob_index_t(offsets.size() - 1); }
    zsize_t size() const;
    zsize_t getFinalSize() const;
    bool is_extended() const { return isExtended; }
    void clear();
    void close();
    bool isClosed() const;

    void setClusterIndex(cluster_index_t idx) { index = idx; }
    cluster_index_t getClusterIndex() const { return index; }

    zsize_t getBlobSize(blob_index_t n) const
    { return zsize_t(offsets[blob_index_type(n)+1].v - offsets[blob_index_type(n)].v); }

    void write_final(std::ostream& out) const;
    void dump_tmp(const std::string& directoryPath);
    void dump(std::ostream& out) const;

  protected:
    CompressionType compression;
    cluster_index_t index;
    bool isExtended;
    Offsets offsets;
    zsize_t _size;
    zsize_t finalSize;
    ClusterData _data;
    std::string tmp_filename;
    mutable pthread_mutex_t m_closedMutex;
    bool closed = false;

  private:
    void write(std::ostream& out) const;
    template<typename OFFSET_TYPE>
    void write_offsets(std::ostream& out) const;
    void write_data(std::ostream& out) const;

};

};

};


#endif //ZIM_WRITER_CLUSTER_H_
