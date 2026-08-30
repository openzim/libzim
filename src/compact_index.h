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

#ifndef ZIM_COMPACT_INDEX_H
#define ZIM_COMPACT_INDEX_H

#include "config.h"
#include "zim_types.h"
#include "buffer.h"

#include <string>
#include <vector>

namespace zim {

// The path pointer list is a table of `articleCount` monotonically
// non-decreasing byte offsets (one per dirent, pointing at where that
// dirent is stored). Storing it as a flat array of raw 8-byte offsets
// wastes a lot of space on data that is almost entirely predictable from
// its neighbours. These two functions compactly encode/decode that table
// as delta+varint bytes, zstd-compressed as a single blob.
//
// This is gated behind Creator::configCompactIndexStructures() /
// Fileheader::usesCompactIndexStructures() -- old readers cannot parse
// this encoding, so it must never be written unless the caller has opted
// in and is prepared to bump the minor version accordingly.

// Encodes a non-decreasing sequence of offsets. Returns the compressed
// bytes only -- the caller is responsible for recording enough
// information (e.g. a length prefix) for a reader to know how many
// compressed bytes to read back.
std::string LIBZIM_PRIVATE_API compactEncodeOffsetList(const std::vector<offset_type>& values);

// Reverses compactEncodeOffsetList(). `count` must be the exact number of
// values that were originally encoded (the caller already knows this --
// it's the article count). Returns a Buffer holding `count` little-endian
// offset_type values, ready to back a BufferReader exactly like the
// uncompressed path pointer table would.
Buffer LIBZIM_PRIVATE_API compactDecodeOffsetList(const char* compressedData, size_t compressedSize, size_t count);

} // namespace zim

#endif // ZIM_COMPACT_INDEX_H
