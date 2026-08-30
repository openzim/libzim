/*
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

#ifndef ZIM_DIRENTREADER_H
#define ZIM_DIRENTREADER_H

#include "_dirent.h"
#include "reader.h"
#include "dirent_chunk.h"
#include "lrucache.h"

#include <memory>
#include <mutex>
#include <vector>

namespace zim
{

// Unlke FileReader and MemoryReader (which read data from a file and memory,
// respectively), DirentReader is a helper class that reads Dirents (rather
// than from a Dirent).
class LIBZIM_PRIVATE_API DirentReader
{
public: // functions
  explicit DirentReader(std::shared_ptr<const Reader> zimReader)
    : mp_zimReader(zimReader),
      m_chunkCache(DIRENT_CHUNK_CACHE_SIZE)
  {}

  // Switches this reader into "dirent chunk" mode (see dirent_chunk.h):
  // from this call on, readDirent()'s offset_t argument is interpreted as
  // a packed (chunkIndex, intraChunkOffset) value rather than a raw
  // absolute file offset. `chunkPtrReader` must read back exactly
  // `chunkCount` little-endian offset_type values (the dirent chunk
  // pointer list); `chunkTableStart` is the absolute file offset where
  // that list begins, which doubles as the end of the last chunk's
  // compressed data (chunks are written back-to-back immediately before
  // it -- see writeDirentChunkPtrList() in writer/creator.cpp).
  // Only called from FileImpl::FileImpl() when
  // Fileheader::usesCompactIndexStructures() is set.
  void enableChunkedDirents(std::unique_ptr<const Reader> chunkPtrReader,
                             uint64_t chunkCount,
                             offset_t chunkTableStart);

  bool hasChunkedDirents() const { return bool(mp_chunkPtrReader); }
  uint64_t getChunkCount() const { return m_chunkCount; }

  // The absolute file offset of the very first dirent chunk -- i.e. the
  // real on-disk position corresponding to entry 0, which packed offsets
  // no longer directly encode. Used by FileImpl::getMimeListEndUpperLimit()
  // to bound where the mimetype list ends. Only valid when
  // hasChunkedDirents() is true and there is at least one chunk.
  offset_t getFirstChunkOffset() const { return getChunkCompressedStart(0); }

  std::shared_ptr<const Dirent> readDirent(offset_t offset);

private: // functions
  bool initDirent(Dirent& dirent, const Buffer& direntData) const;
  std::shared_ptr<const Dirent> readDirentFromChunk(offset_t packedOffset);
  std::shared_ptr<const Buffer> getChunk(uint64_t chunkIndex);
  offset_t getChunkCompressedStart(uint64_t chunkIndex) const;
  offset_t getChunkCompressedEnd(uint64_t chunkIndex) const;

private: // data
  std::shared_ptr<const Reader> mp_zimReader;
  std::vector<char> m_buffer;
  std::mutex m_bufferMutex;

  // Dirent-chunk mode (see enableChunkedDirents() above). mp_chunkPtrReader
  // is null when dirents are stored the old (unchunked) way.
  std::unique_ptr<const Reader> mp_chunkPtrReader;
  uint64_t m_chunkCount = 0;
  offset_t m_chunkTableStart{0};

  mutable lru_cache<uint64_t, std::shared_ptr<const Buffer>, DirentChunkMemorySize> m_chunkCache;
  mutable std::mutex m_chunkCacheMutex;
};

} // namespace zim

#endif // ZIM_DIRENTREADER_H
