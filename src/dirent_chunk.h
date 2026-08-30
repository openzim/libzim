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

#ifndef ZIM_DIRENT_CHUNK_H
#define ZIM_DIRENT_CHUNK_H

#include "config.h"
#include "zim_types.h"
#include "buffer.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

namespace zim {

// When Creator::configCompactIndexStructures() is enabled, the dirent
// table itself (one variable-length record per entry, the single largest
// uncompressed structure in a typical ZIM file -- ~13.6% of total size on
// a real Wikipedia ZIM) is no longer appended one dirent at a time
// directly to the output file. Instead, dirents are grouped into
// DIRENT_CHUNK_TARGET_SIZE-ish (uncompressed) chunks, each independently
// zstd-compressed as its own frame -- mirroring how content clusters are
// already compressed.
//
// A dirent's "offset" (the value stored in the path pointer list, see
// compact_index.h) becomes a packed value combining a chunk index and a
// byte offset within that chunk's *decompressed* data:
//
//   packedOffset = (chunkIndex << DIRENT_CHUNK_SHIFT) | intraChunkOffset
//
// DIRENT_CHUNK_SHIFT reserves 20 bits (1MiB) for the intra-chunk offset --
// 16x the ~64KiB target chunk size, comfortable headroom so a chunk would
// have to massively overshoot its target before the packing scheme
// breaks (checked for at write time, see packDirentOffset()).
//
// Dirents are written into chunks in the same (URL-sorted) order they
// always have been: chunk index and intra-chunk offset both only ever
// increase as we walk that order, so packed offsets come out strictly
// increasing overall -- required because they may be re-encoded by
// compactEncodeOffsetList() (see compact_index.h), which assumes
// non-decreasing input.
//
// This is gated behind the same Creator::configCompactIndexStructures() /
// Fileheader::usesCompactIndexStructures() flag used for the path pointer
// list and title listing (see compact_index.h) -- old readers cannot
// parse this encoding, and there is no released version of libzim yet
// that understands *any* of the "compact index structures" tier, so there
// is no benefit to splitting this into a separate opt-in/version.

constexpr unsigned DIRENT_CHUNK_SHIFT = 20;
constexpr offset_type DIRENT_CHUNK_INTRA_MASK = (offset_type(1) << DIRENT_CHUNK_SHIFT) - 1;

// Target *uncompressed* size of a dirent chunk. A chunk is closed (and
// compressed) as soon as it reaches this size, always on a dirent
// boundary -- a dirent is never split across two chunks.
constexpr size_type DIRENT_CHUNK_TARGET_SIZE = 64 * 1024;

inline offset_type packDirentOffset(uint64_t chunkIndex, size_t intraChunkOffset)
{
  if (offset_type(intraChunkOffset) > DIRENT_CHUNK_INTRA_MASK) {
    // A single dirent chunk grew past 1MiB of uncompressed data -- 16x
    // its ~64KiB target. This should never happen in practice (it would
    // require a dirent chunk holding either an enormous number of
    // dirents or some with pathological path/title lengths); refuse to
    // silently produce a corrupt packed offset.
    throw std::runtime_error("packDirentOffset: dirent chunk grew past the maximum representable size");
  }
  return (offset_type(chunkIndex) << DIRENT_CHUNK_SHIFT) | offset_type(intraChunkOffset);
}

inline uint64_t direntChunkIndex(offset_type packedOffset)
{
  return packedOffset >> DIRENT_CHUNK_SHIFT;
}

inline size_t direntChunkIntraOffset(offset_type packedOffset)
{
  return size_t(packedOffset & DIRENT_CHUNK_INTRA_MASK);
}

// Compresses one dirent chunk's raw, concatenated dirent bytes into a
// self-delimited zstd frame (the frame header embeds the uncompressed
// size, so decompressDirentChunk() doesn't need it passed separately).
std::string LIBZIM_PRIVATE_API compressDirentChunk(const char* data, size_t size);

// Decompresses a dirent chunk given its exact compressed byte range --
// known from two consecutive entries in the dirent chunk pointer list (or
// the end-of-chunk-table marker for the last chunk; see fileimpl.cpp).
Buffer LIBZIM_PRIVATE_API decompressDirentChunk(const char* compressedData, size_t compressedSize);

// Cost estimation (in bytes of decompressed data) for caching decompressed
// dirent chunks, mirroring ClusterMemorySize in cluster.h.
struct DirentChunkMemorySize {
  static size_t cost(const std::shared_ptr<const Buffer>& buffer) {
    return buffer->size().v;
  }
};

} // namespace zim

#endif // ZIM_DIRENT_CHUNK_H
