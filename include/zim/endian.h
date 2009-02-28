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

/// Returns true, if machine is big-endian (high byte first).
/// e.g. PowerPC
inline bool isBigEndian()
{
  const int i = 1;
  return *reinterpret_cast<const int8_t*>(&i) == 0;
}

/// Returns true, if machine is little-endian (low byte first).
/// e.g. x86
inline bool isLittleEndian()
{
  const int i = 1;
  return *reinterpret_cast<const int8_t*>(&i) == 1;
}

////////////////////////////////////////////////////////////////////////
template <typename T>
T fromLittleEndian(const T* ptr, bool fromBigEndian = isBigEndian())
{
  if (fromBigEndian)
  {
    T ret;
    std::reverse_copy(reinterpret_cast<const int8_t*>(ptr),
                      reinterpret_cast<const int8_t*>(ptr) + sizeof(T),
                      reinterpret_cast<int8_t*>(&ret));
    return ret;
  }
  else
  {
    return *ptr;
  }
}

template <typename T>
T fromLittleEndian(T* ptr, bool fromBigEndian = isBigEndian())
{ return fromLittleEndian(static_cast<const T*>(ptr), fromBigEndian); }

template <typename T>
T fromLittleEndian(const void* ptr, bool fromBigEndian = isBigEndian())
{ return fromLittleEndian(static_cast<const T*>(ptr), fromBigEndian); }

template <typename T>
T fromLittleEndian(const T& t, bool fromBigEndian = isBigEndian())
{ return fromLittleEndian(static_cast<const T*>(&t), fromBigEndian); }

template <typename T>
T fromLittleEndian(std::istream& in, bool fromBigEndian = isBigEndian())
{
  T data;
  in.read(reinterpret_cast<char*>(&data), sizeof(T));
  return fromLittleEndian(&data, fromBigEndian);
}

////////////////////////////////////////////////////////////////////////
template <typename T>
T fromBigEndian(const T* ptr, bool fromBigEndian_ = isBigEndian())
{
  if (fromBigEndian_)
  {
    return *ptr;
  }
  else
  {
    T ret;
    std::reverse_copy(reinterpret_cast<const int8_t*>(ptr),
                      reinterpret_cast<const int8_t*>(ptr) + sizeof(T),
                      reinterpret_cast<int8_t*>(&ret));
    return ret;
  }
}

template <typename T>
T fromBigEndian(T* ptr, bool fromBigEndian_ = isBigEndian())
{ return fromBigEndian(static_cast<const T*>(ptr), fromBigEndian_); }

template <typename T>
T fromBigEndian(const void* ptr, bool fromBigEndian_ = isBigEndian())
{ return fromBigEndian(static_cast<const T*>(ptr), fromBigEndian_); }

template <typename T>
T fromBigEndian(const T& t, bool fromBigEndian = isBigEndian())
{ return fromBigEndian(static_cast<const T*>(&t), fromBigEndian); }

template <typename T>
T fromBigEndian(std::istream& in, bool fromBigEndian_ = isBigEndian())
{
  T data;
  in.read(reinterpret_cast<char*>(&data), sizeof(T));
  return fromBigEndian(&data, fromBigEndian_);
}

#endif // ENDIAN_H

