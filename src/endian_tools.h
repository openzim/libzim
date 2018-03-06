/*
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

#ifndef ENDIAN_H
#define ENDIAN_H

#include <algorithm>
#include <iostream>
#include <zim/zim.h>

namespace zim
{

template<typename T, size_t N>
struct ToLittleEndianImpl;

template<typename T>
struct ToLittleEndianImpl<T, 2>{
  static void write(const T& d, char* dst) {
    uint16_t v = static_cast<uint16_t>(d);
    dst[0] = static_cast<uint8_t>(v);
    dst[1] = static_cast<uint8_t>(v>>8);
  }
};

template<typename T>
struct ToLittleEndianImpl<T, 4>{
  static void write(const T& d, char* dst) {
    uint32_t v = static_cast<uint32_t>(d);
    dst[0] = static_cast<uint8_t>(v);
    dst[1] = static_cast<uint8_t>(v>>8);
    dst[2] = static_cast<uint8_t>(v>>16);
    dst[3] = static_cast<uint8_t>(v>>24);
}
};

template<typename T>
struct ToLittleEndianImpl<T, 8>{
  static void write(const T& d, char* dst) {
    uint64_t v = static_cast<uint64_t>(d);
    dst[0] = static_cast<uint8_t>(v);
    dst[1] = static_cast<uint8_t>(v>>8);
    dst[2] = static_cast<uint8_t>(v>>16);
    dst[3] = static_cast<uint8_t>(v>>24);
    dst[4] = static_cast<uint8_t>(v>>32);
    dst[5] = static_cast<uint8_t>(v>>40);
    dst[6] = static_cast<uint8_t>(v>>48);
    dst[7] = static_cast<uint8_t>(v>>56);
  }
};

////////////////////////////////////////////////////////////////////////
template <typename T>
inline void toLittleEndian(T d, char* dst)
{
  ToLittleEndianImpl<T, sizeof(T)>::write(d, dst);
}

template <typename T>
inline T fromLittleEndian(const char* ptr)
{
  T ret = 0;
  for(size_t i=0; i<sizeof(T); i++) {
    ret |= (static_cast<T>(static_cast<uint8_t>(ptr[i])) << (i*8));
  }
  return ret;
}

}

#endif // ENDIAN_H

