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

#include "cluster.h"
#include <zim/blob.h>
#include <zim/error.h>
#include "reader.h"
#include "endian_tools.h"
#include "readerdatastreamwrapper.h"
#include "bufdatastream.h"
#include "decodeddatastream.h"
#include <algorithm>
#include <stdlib.h>
#include <sstream>

#include "compression.h"
#include "log.h"

#include "config.h"

log_define("zim.cluster")

#define log_debug1(e)

namespace zim
{

namespace
{

template<typename Decoder>
std::unique_ptr<IDataStream>
makeDecodedDataStream(std::unique_ptr<IDataStream> data, size_t size)
{
  const auto dds(new DecodedDataStream<Decoder>(std::move(data), size));
  return std::unique_ptr<IDataStream>(dds);
}

std::unique_ptr<IDataStream>
getUncompressedClusterDataStream(std::shared_ptr<const Reader> reader, CompressionType comp)
{
  std::unique_ptr<IDataStream> rdsw(new ReaderDataStreamWrapper(reader));
  const size_t size = reader->size().v;
  switch (comp)
  {
    case zimcompLzma:
      return makeDecodedDataStream<LZMA_INFO>(std::move(rdsw), size);

    case zimcompZip:
#if defined(ENABLE_ZLIB)
      return makeDecodedDataStream<ZIP_INFO>(std::move(rdsw), size);
#else
      throw std::runtime_error("zlib not enabled in this library");
#endif

    case zimcompZstd:
      return makeDecodedDataStream<ZSTD_INFO>(std::move(rdsw), size);

    case zimcompBzip2:
      throw std::runtime_error("bzip2 not enabled in this library");

    default:
      throw ZimFileFormatError("Invalid compression flag");
  }
}

} // unnamed namespace

////////////////////////////////////////////////////////////////////////////////
// Cluster
////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<Cluster>
Cluster::read(const Reader& zimReader, offset_t clusterOffset)
{
  const uint8_t clusterInfo = zimReader.read_uint<uint8_t>(clusterOffset);

  const CompressionType comp = static_cast<CompressionType>(clusterInfo & 0x0F);
  const bool extended = clusterInfo & 0x10;

  const std::shared_ptr<const Reader> clusterReader(zimReader.sub_reader(clusterOffset+offset_t(1)));

  if (comp == zimcompDefault || comp == zimcompNone)
    return std::make_shared<NonCompressedCluster>(clusterReader, extended);

  return std::make_shared<CompressedCluster>(clusterReader, comp, extended);
}

////////////////////////////////////////////////////////////////////////////////
// NonCompressedCluster
////////////////////////////////////////////////////////////////////////////////

NonCompressedCluster::NonCompressedCluster(std::shared_ptr<const Reader> reader_, bool isExtended)
  : Cluster(isExtended),
    reader(reader_),
    startOffset(0)
{
  if (isExtended) {
    startOffset = read_header<uint64_t>();
  } else {
    startOffset = read_header<uint32_t>();
  }
  reader = reader->sub_reader(startOffset, zsize_t(offsets.back().v));
}

/* This return the number of char read */
template<typename OFFSET_TYPE>
offset_t NonCompressedCluster::read_header()
{
  // read first offset, which specifies, how many offsets we need to read
  OFFSET_TYPE offset = reader->read_uint<OFFSET_TYPE>(offset_t(0));

  const size_t S = sizeof(OFFSET_TYPE);
  size_t n_offset = offset / S;
  const offset_t data_address(offset);

  // read offsets
  offsets.clear();
  offsets.reserve(n_offset);
  offsets.push_back(offset_t(0));

  const Blob clusterHeader = reader->read_blob(offset_t(S), zsize_t(offset-S));
  BufDataStream bds(clusterHeader.data(), clusterHeader.size());
  while (--n_offset)
  {
    OFFSET_TYPE new_offset = bds.read<OFFSET_TYPE>();
    ASSERT(new_offset, >=, offset);
    ASSERT(new_offset, <=, reader->size().v);

    offset = new_offset;
    offsets.push_back(offset_t(offset - data_address.v));
  }
  return data_address;
}

zsize_t NonCompressedCluster::getBlobSize(blob_index_t n) const
{
    if (blob_index_type(n)+1 >= offsets.size()) throw ZimFileFormatError("blob index out of range");
    return zsize_t(offsets[blob_index_type(n)+1].v - offsets[blob_index_type(n)].v);
}

Blob NonCompressedCluster::getBlob(blob_index_t n) const
{
  if (n < count()) {
    auto blobSize = getBlobSize(n);
    if (blobSize.v > SIZE_MAX) {
      return Blob();
    }
    return reader->read_blob(offsets[blob_index_type(n)], blobSize);;
  } else {
    return Blob();
  }
}

Blob NonCompressedCluster::getBlob(blob_index_t n, offset_t offset, zsize_t size) const
{
  if (n < count()) {
    const auto blobSize = getBlobSize(n);
    if ( offset.v > blobSize.v ) {
      return Blob();
    }
    size = std::min(size, zsize_t(blobSize.v-offset.v));
    if (size.v > SIZE_MAX) {
      return Blob();
    }
    offset += offsets[blob_index_type(n)];
    return reader->read_blob(offset, size);
  } else {
    return Blob();
  }
}

////////////////////////////////////////////////////////////////////////////////
// CompressedCluster
////////////////////////////////////////////////////////////////////////////////

CompressedCluster::CompressedCluster(std::shared_ptr<const Reader> reader, CompressionType comp, bool isExtended)
  : Cluster(isExtended)
  , ds_(getUncompressedClusterDataStream(reader, comp))
  , compression_(comp)
{
  ASSERT(compression_, >, zimcompNone);

  if ( isExtended )
    readHeader<uint64_t>(*ds_);
  else
    readHeader<uint32_t>(*ds_);
}

bool
CompressedCluster::isCompressed() const
{
  return true;
}

CompressionType
CompressedCluster::getCompression() const
{
  return compression_;
}

blob_index_t
CompressedCluster::count() const
{
  return blob_index_t(blobSizes_.size());
}

zsize_t
CompressedCluster::getBlobSize(blob_index_t n) const
{
  ASSERT(n.v, <, blobSizes_.size());
  return zsize_t(blobSizes_[n.v]);
}

offset_t
CompressedCluster::getBlobOffset(blob_index_t n) const
{
  throw std::logic_error("CompressedCluster::getBlobOffset() should never be called");
}

template<typename OFFSET_TYPE>
void
CompressedCluster::readHeader(IDataStream& ds)
{
  OFFSET_TYPE offset = ds.read<OFFSET_TYPE>();

  size_t n_offset = offset / sizeof(OFFSET_TYPE);

  while (--n_offset)
  {
    OFFSET_TYPE new_offset = ds.read<OFFSET_TYPE>();
    ASSERT(new_offset, >=, offset);

    blobSizes_.push_back(new_offset - offset);
    offset = new_offset;
  }
}

void
CompressedCluster::ensureBlobIsDecompressed(blob_index_t n) const
{
  for ( size_t i = blobs_.size(); i <= n.v; ++i )
    blobs_.push_back(ds_->readBlob(getBlobSize(blob_index_t(i)).v));
}

Blob
CompressedCluster::getBlob(blob_index_t n) const
{
  std::lock_guard<std::mutex> lock(blobAccessMutex_);
  ensureBlobIsDecompressed(n);
  ASSERT(n.v, <, blobs_.size());
  return blobs_[n.v];
}

Blob
CompressedCluster::getBlob(blob_index_t n, offset_t offset, zsize_t size) const
{
  std::lock_guard<std::mutex> lock(blobAccessMutex_);
  ensureBlobIsDecompressed(n);
  ASSERT(n.v, <, blobs_.size());
  return blobs_[n.v].subBlob(offset.v, size.v);
}

} // namespace zim
