/*
 * Copyright (C) 2017-2020 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2006 Tommi Maekitalo
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

#include "_dirent.h"
#include "direntreader.h"
#include "dirent_chunk.h"
#include <zim/zim.h>
#include <zim/error.h>
#include "buffer.h"
#include "bufferstreamer.h"
#include "log.h"
#include <cstring>

log_define("zim.dirent")

namespace zim
{
  //////////////////////////////////////////////////////////////////////
  // Dirent
  //

  const uint16_t Dirent::redirectMimeType;
  const uint16_t Dirent::linktargetMimeType;
  const uint16_t Dirent::deletedMimeType;

  bool DirentReader::initDirent(Dirent& dirent, const Buffer& direntData) const
  {
    BufferStreamer reader(direntData);
    uint16_t mimeType = reader.read<uint16_t>();
    bool redirect = (mimeType == Dirent::redirectMimeType);
    bool linktarget = (mimeType == Dirent::linktargetMimeType);
    bool deleted = (mimeType == Dirent::deletedMimeType);
    uint8_t extraLen = reader.read<uint8_t>();
    char ns = reader.read<char>();
    uint32_t version = reader.read<uint32_t>();
    dirent.setVersion(version);

    if (redirect)
    {
      entry_index_type redirectIndex(reader.read<entry_index_type>());

      log_debug("redirectIndex=" << redirectIndex);

      dirent.setRedirect(entry_index_t(redirectIndex));
    }
    else if (linktarget || deleted)
    {
      log_debug("linktarget or deleted entry");
      dirent.setItem(mimeType, cluster_index_t(0), blob_index_t(0));
    }
    else
    {
      log_debug("read article entry");

      uint32_t clusterNumber = reader.read<uint32_t>();
      uint32_t blobNumber = reader.read<uint32_t>();

      log_debug("mimeType=" << mimeType << " clusterNumber=" << clusterNumber << " blobNumber=" << blobNumber);

      dirent.setItem(mimeType, cluster_index_t(clusterNumber), blob_index_t(blobNumber));
    }

    std::string path;
    std::string title;
    std::string parameter;

    log_debug("read path, title and parameters");

    size_type path_size = strnlen(
      reader.current(),
      reader.left().v - extraLen
    );
    if (path_size >= reader.left().v) {
      return false;
    }
    path = std::string(reader.current(), path_size);
    reader.skip(zsize_t(path_size + 1));

    size_type title_size = strnlen(
      reader.current(),
      reader.left().v - extraLen
    );
    if (title_size >= reader.left().v) {
      return false;
    }
    title = std::string(reader.current(), title_size);
    reader.skip(zsize_t(title_size+1));

    if (extraLen > reader.left().v) {
      return false;
    }
    parameter = std::string(reader.current(), extraLen);
    dirent.setPath(ns, path);
    dirent.setTitle(title);
    dirent.setParameter(parameter);
    return true;
  }

  void DirentReader::enableChunkedDirents(std::unique_ptr<const Reader> chunkPtrReader,
                                           uint64_t chunkCount,
                                           offset_t chunkTableStart)
  {
    mp_chunkPtrReader = std::move(chunkPtrReader);
    m_chunkCount = chunkCount;
    m_chunkTableStart = chunkTableStart;
  }

  offset_t DirentReader::getChunkCompressedStart(uint64_t chunkIndex) const
  {
    return offset_t(mp_chunkPtrReader->read_uint<offset_type>(offset_t(sizeof(offset_type)*chunkIndex)));
  }

  offset_t DirentReader::getChunkCompressedEnd(uint64_t chunkIndex) const
  {
    if (chunkIndex + 1 < m_chunkCount) {
      return getChunkCompressedStart(chunkIndex+1);
    }
    // The last chunk's compressed data ends exactly where the dirent
    // chunk pointer list begins -- chunks are written back-to-back
    // immediately before it (see writeDirentChunkPtrList()).
    return m_chunkTableStart;
  }

  std::shared_ptr<const Buffer> DirentReader::getChunk(uint64_t chunkIndex)
  {
    {
      std::lock_guard<std::mutex> l(m_chunkCacheMutex);
      auto v = m_chunkCache.get(chunkIndex);
      if (v.hit()) {
        return v.value();
      }
    }

    const offset_t compressedStart = getChunkCompressedStart(chunkIndex);
    const offset_t compressedEnd = getChunkCompressedEnd(chunkIndex);
    if (compressedEnd.v <= compressedStart.v) {
      throw ZimFileFormatError("Invalid dirent chunk pointer");
    }
    const zsize_t compressedSize(compressedEnd.v - compressedStart.v);

    const auto compressedBuf = mp_zimReader->get_buffer(compressedStart, compressedSize);
    auto decompressed = std::make_shared<const Buffer>(
      decompressDirentChunk(compressedBuf.data(), compressedSize.v));

    std::lock_guard<std::mutex> l(m_chunkCacheMutex);
    m_chunkCache.put(chunkIndex, decompressed);
    return decompressed;
  }

  std::shared_ptr<const Dirent> DirentReader::readDirentFromChunk(offset_t packedOffset)
  {
    const uint64_t chunkIdx = direntChunkIndex(packedOffset.v);
    if (chunkIdx >= m_chunkCount) {
      throw ZimFileFormatError("Invalid dirent chunk index");
    }
    const size_t intraOffset = direntChunkIntraOffset(packedOffset.v);

    const auto chunkData = getChunk(chunkIdx);
    if (intraOffset >= chunkData->size().v) {
      throw ZimFileFormatError("Invalid intra-chunk dirent offset");
    }

    // The whole decompressed chunk is already fully in memory, so (unlike
    // the unchunked path below) there's no point in guessing a small
    // buffer size and growing it -- just hand initDirent() everything
    // from intraOffset to the end of the chunk in one go. By construction
    // a dirent never straddles a chunk boundary (see writeChunkedDirents()
    // in writer/creator.cpp), so this always contains the whole dirent.
    const size_t available = chunkData->size().v - intraOffset;
    const Buffer direntBuf = chunkData->sub_buffer(offset_t(intraOffset), zsize_t(available));
    auto dirent = std::make_shared<Dirent>();
    if (!initDirent(*dirent, direntBuf)) {
      throw ZimFileFormatError("Invalid dirent data in dirent chunk");
    }
    return dirent;
  }

  std::shared_ptr<const Dirent> DirentReader::readDirent(offset_t offset)
  {
    if (mp_chunkPtrReader) {
      return readDirentFromChunk(offset);
    }

    const auto totalSize = mp_zimReader->size();
    if (offset.v >= totalSize.v) {
      throw ZimFileFormatError("Invalid dirent pointer");
    }

    // We don't know the size of the dirent because it depends of the size of
    // the title, path and extra parameters.
    // This is a pity but we have no choice.
    // We cannot take a buffer of the size of the file, it would be really
    // inefficient. Let's do try, catch and retry while choosing a smart value
    // for the buffer size. Most dirent will be "Article" entry (header's size
    // == 16) without extra parameters. Let's hope that path + title size will
    // be < 256 and if not try again with a bigger size.

    size_t bufferSize(std::min(size_type(256), mp_zimReader->size().v-offset.v));
    auto dirent = std::make_shared<Dirent>();
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    for ( ; ; bufferSize += 256 ) {
      m_buffer.reserve(bufferSize);
      mp_zimReader->read(m_buffer.data(), offset, zsize_t(bufferSize));
      if ( initDirent(*dirent, Buffer::makeBuffer(m_buffer.data(), zsize_t(bufferSize))) )
        return dirent;
    }
  }

  std::string Dirent::getLongPath() const
  {
    log_trace("Dirent::getLongPath()");
    log_debug("namespace=" << getNamespace() << " title=" << getTitle());

    return std::string(1, getNamespace()) + '/' + getPath();
  }

}
