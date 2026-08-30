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

#include "compact_index.h"
#include "endian_tools.h"

#include <zstd.h>

#include <stdexcept>

namespace zim {

namespace {

void appendVarint(std::string& out, uint64_t value)
{
  while (value >= 0x80) {
    out.push_back(static_cast<char>((value & 0x7f) | 0x80));
    value >>= 7;
  }
  out.push_back(static_cast<char>(value));
}

uint64_t readVarint(const char*& p, const char* end)
{
  uint64_t result = 0;
  int shift = 0;
  while (p < end) {
    const uint8_t byte = static_cast<uint8_t>(*p++);
    result |= static_cast<uint64_t>(byte & 0x7f) << shift;
    if (!(byte & 0x80)) {
      return result;
    }
    shift += 7;
    if (shift > 63) {
      throw std::runtime_error("compactDecodeOffsetList: varint too long");
    }
  }
  throw std::runtime_error("compactDecodeOffsetList: truncated varint");
}

// zstd's simple one-shot API embeds the uncompressed content size in the
// frame header (ZSTD_compress does this by default), so we can query it
// back at decode time instead of having to store it separately.
constexpr int ZSTD_COMPACT_INDEX_LEVEL = 19;

} // unnamed namespace

std::string compactEncodeOffsetList(const std::vector<offset_type>& values)
{
  std::string raw;
  raw.reserve(values.size()); // deltas are usually small, 1 byte each
  offset_type prev = 0;
  for (const auto value : values) {
    if (value < prev) {
      throw std::runtime_error("compactEncodeOffsetList: values must be non-decreasing");
    }
    appendVarint(raw, value - prev);
    prev = value;
  }

  const size_t bound = ZSTD_compressBound(raw.size());
  std::string compressed(bound, '\0');
  const size_t compressedSize = ZSTD_compress(
    &compressed[0], bound,
    raw.data(), raw.size(),
    ZSTD_COMPACT_INDEX_LEVEL
  );
  if (ZSTD_isError(compressedSize)) {
    throw std::runtime_error(
      std::string("compactEncodeOffsetList: zstd compression failed: ") +
      ZSTD_getErrorName(compressedSize)
    );
  }
  compressed.resize(compressedSize);
  return compressed;
}

Buffer compactDecodeOffsetList(const char* compressedData, size_t compressedSize, size_t count)
{
  const unsigned long long rawSize = ZSTD_getFrameContentSize(compressedData, compressedSize);
  if (rawSize == ZSTD_CONTENTSIZE_ERROR || rawSize == ZSTD_CONTENTSIZE_UNKNOWN) {
    throw std::runtime_error("compactDecodeOffsetList: cannot determine uncompressed size");
  }

  std::vector<char> raw(rawSize);
  if (rawSize > 0) {
    const size_t got = ZSTD_decompress(raw.data(), rawSize, compressedData, compressedSize);
    if (ZSTD_isError(got)) {
      throw std::runtime_error(
        std::string("compactDecodeOffsetList: zstd decompression failed: ") +
        ZSTD_getErrorName(got)
      );
    }
    if (got != rawSize) {
      throw std::runtime_error("compactDecodeOffsetList: unexpected decompressed size");
    }
  }

  const size_t outSize = count * sizeof(offset_type);
  std::unique_ptr<char[]> out(new char[outSize]);
  const char* p = raw.data();
  const char* end = p + raw.size();
  offset_type prev = 0;
  for (size_t i = 0; i < count; ++i) {
    const uint64_t delta = readVarint(p, end);
    prev += delta;
    toLittleEndian(prev, out.get() + i * sizeof(offset_type));
  }

  Buffer::DataPtr dataPtr(out.release(), std::default_delete<char[]>());
  return Buffer::makeBuffer(dataPtr, zsize_t(outSize));
}

} // namespace zim
