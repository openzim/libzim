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

#include "../zim_types.h"

namespace zim {

namespace writer {


class Cluster {
  friend std::ostream& operator<< (std::ostream& out, const Cluster& blobImpl);
  typedef std::vector<size_type> Offsets;
  typedef std::vector<char> Data;


  public:
    Cluster();

    void setCompression(CompressionType c) { compression = c; }
    CompressionType getCompression() const { return compression; }
    blob_index_t count() const  { return blob_index_t(offsets.size() - 1); }
    offset_type size() const   { return offsets.size() * sizeof(size_type) + _data.size(); }
    void clear();

    size_type getBlobSize(blob_index_t n) const { return offsets[n.v+1] - offsets[n.v]; }

    void addBlob(const Blob& blob);
    void addBlob(const char* data, unsigned size);

    void write(std::ostream& out) const;

  private:
    CompressionType compression;
    Offsets offsets;
    Data _data;

};

std::ostream& operator<< (std::ostream& out, const Cluster& cluster);

};

};


#endif //ZIM_WRITER_CLUSTER_H_
