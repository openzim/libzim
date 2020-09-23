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

#ifndef ZIM_IDATASTREAM_H
#define ZIM_IDATASTREAM_H

#include <exception>
#include <memory>

#include "endian_tools.h"
#include "reader.h"

namespace zim
{

// IDataStream is a simple interface for sequential iteration over a stream
// of values of built-in/primitive types and/or opaque binary objects (blobs).
// An example usage:
//
//   void foo(IDataStream& s)
//   {
//     const uint32_t n = s.read<uint32_t>();
//     for(uint32_t i=0; i < n; ++i)
//     {
//        const uint16_t blobSize = s.read<uint16_t>();
//        IDataStream::Blob blob = s.readBlob(blobSize);
//        bar(blob, blobSize);
//     }
//   }
//
class IStreamReader
{
public: // functions
  virtual ~IStreamReader() = default;

  // Reads a value of the said type from the stream
  //
  // For best portability this function should be used with types of known
  // bit-width (int32_t, uint16_t, etc) rather than builtin types with
  // unknown bit-width (int, unsigned, etc).
  template<typename T> T read();

  // Reads a blob of the specified size from the stream
  virtual std::unique_ptr<const Reader> sub_reader(zsize_t size);

private: // virtual methods
  // Reads exactly 'nbytes' bytes into the provided buffer 'buf'
  // (which must be at least that big). Throws an exception if
  // more bytes are requested than can be retrieved.
  virtual void readImpl(char* buf, zsize_t nbytes) = 0;
};

////////////////////////////////////////////////////////////////////////////////
// Implementation of IDataStream
////////////////////////////////////////////////////////////////////////////////

// XXX: Assuming that opaque binary data retrieved via 'readImpl()'
// XXX: is encoded in little-endian form.
template<typename T>
inline T
IStreamReader::read()
{
  const zsize_t N(sizeof(T));
  char buf[N.v];
  readImpl(buf, N);
  return fromLittleEndian<T>(buf); // XXX: This handles only integral types
}

} // namespace zim

#endif // ZIM_IDATASTREAM_H
