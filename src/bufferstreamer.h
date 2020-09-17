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

#ifndef ZIM_BUFFERSTREAMER_H
#define ZIM_BUFFERSTREAMER_H

#include "debug.h"

#include <string.h>

namespace zim
{

class BufferStreamer
{
public: // functions
  BufferStreamer(const Buffer& buffer, zsize_t size)
    : m_buffer(buffer),
      m_current(buffer.data()),
      m_size(size)
  {}

  explicit BufferStreamer(const Buffer& buffer)
    : BufferStreamer(buffer, buffer.size())
  {}

  // Reads a value of the said type from the stream
  //
  // For best portability this function should be used with types of known
  // bit-width (int32_t, uint16_t, etc) rather than builtin types with
  // unknown bit-width (int, unsigned, etc).
  template<typename T> T read()
  {
    const size_t N(sizeof(T));
    char buf[N];
    memcpy(buf, m_current, N);
    skip(zsize_t(N));
    return fromLittleEndian<T>(buf); // XXX: This handles only integral types
  }

  const char* current() const {
    return m_current;
  }

  zsize_t left() const {
    return m_size;
  }

  void skip(zsize_t nbBytes) {
    m_current += nbBytes.v;
    m_size -= nbBytes;
  }

private: // data
  const Buffer m_buffer;
  const char* m_current;
  zsize_t m_size;
};

} // namespace zim

#endif // ZIM_BUFDATASTREAM_H
