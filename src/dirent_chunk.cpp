/*
 * Copyright (C) 2026 Kiwix
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
 */

#include "dirent_chunk.h"

#include <zstd.h>

namespace zim {

namespace {

// Matches the level used for the path pointer list in compact_index.cpp:
// this is a one-time write cost, and the whole point of this feature is
// maximizing the compression ratio of what is otherwise the largest
// uncompressed structure in the file.
constexpr int ZSTD_DIRENT_CHUNK_LEVEL = 19;

} // unnamed namespace

std::string compressDirentChunk(const char* data, size_t size)
{
  const size_t bound = ZSTD_compressBound(size);
  std::string compressed(bound, '\0');
  const size_t compressedSize = ZSTD_compress(
    &compressed[0], bound,
    data, size,
    ZSTD_DIRENT_CHUNK_LEVEL
  );
  if (ZSTD_isError(compressedSize)) {
    throw std::runtime_error(
      std::string("compressDirentChunk: zstd compression failed: ") +
      ZSTD_getErrorName(compressedSize)
    );
  }
  compressed.resize(compressedSize);
  return compressed;
}

Buffer decompressDirentChunk(const char* compressedData, size_t compressedSize)
{
  const unsigned long long rawSize = ZSTD_getFrameContentSize(compressedData, compressedSize);
  if (rawSize == ZSTD_CONTENTSIZE_ERROR || rawSize == ZSTD_CONTENTSIZE_UNKNOWN) {
    throw std::runtime_error("decompressDirentChunk: cannot determine uncompressed size");
  }

  std::unique_ptr<char[]> out(new char[rawSize]);
  if (rawSize > 0) {
    const size_t got = ZSTD_decompress(out.get(), rawSize, compressedData, compressedSize);
    if (ZSTD_isError(got)) {
      throw std::runtime_error(
        std::string("decompressDirentChunk: zstd decompression failed: ") +
        ZSTD_getErrorName(got)
      );
    }
    if (got != rawSize) {
      throw std::runtime_error("decompressDirentChunk: unexpected decompressed size");
    }
  }

  Buffer::DataPtr dataPtr(out.release(), std::default_delete<char[]>());
  return Buffer::makeBuffer(dataPtr, zsize_t(rawSize));
}

} // namespace zim
