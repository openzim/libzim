/*
 * Copyright (C) 2018-2021 Matthieu Gautier <mgautier@kymeria.fr>
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


#ifndef ZIM_TYPES_H
#define ZIM_TYPES_H

#include <zim/zim.h>

#include <ostream>

#ifdef __GNUC__
#define PACKED __attribute__((packed))
#else
#define PACKED
#endif

template<typename B, typename S>
struct REAL_TYPEDEF{
  typedef B base_type;
  typedef S SELF;

  B v;
  REAL_TYPEDEF() : v(0) {};
  explicit REAL_TYPEDEF(B v) : v(v) {};
  explicit inline operator bool() const { return v != 0; }
  explicit inline operator B() const { return v; }

  inline bool operator==(const REAL_TYPEDEF<B, S>& rhs) const
  { return v == rhs.v; }

  inline REAL_TYPEDEF<B, S>& operator++()
  { v++; return *this; }

  inline REAL_TYPEDEF<B, S> operator++(int)
  { return REAL_TYPEDEF<B, S>(v++); }
} PACKED;

template<typename B, typename S>
std::ostream& operator<<(std::ostream& os, const REAL_TYPEDEF<B, S>& obj)
{
    os << obj.v;
    return os;
}

namespace zim {

#define TYPEDEF(NAME, TYPE) struct NAME : public REAL_TYPEDEF<TYPE, NAME> {          \
explicit NAME(TYPE v=0) : REAL_TYPEDEF<TYPE, NAME>(v) {}; } PACKED;                  \
static_assert(sizeof(NAME) == sizeof(TYPE), "");                                     \
inline NAME& operator+= (NAME& lhs, const NAME& rhs) { lhs.v += rhs.v; return lhs; } \
inline NAME& operator+= (NAME& lhs, const TYPE& rhs) { lhs.v += rhs; return lhs; }   \
inline NAME operator+(NAME lhs, const NAME& rhs) { lhs += rhs; return lhs; }         \
inline NAME& operator-=(NAME& lhs, const NAME& rhs) { lhs.v -= rhs.v; return lhs; }  \
inline NAME operator-(NAME lhs, const NAME& rhs) { lhs -= rhs; return lhs; }         \
inline bool operator< (const NAME& lhs, const NAME& rhs) { return lhs.v < rhs.v; }   \
inline bool operator> (const NAME& lhs, const NAME& rhs) { return rhs < lhs; }       \
inline bool operator<=(const NAME& lhs, const NAME& rhs) { return !(lhs > rhs); }    \
inline bool operator>=(const NAME& lhs, const NAME& rhs) { return !(lhs < rhs); }    \
inline bool operator!=(const NAME& lhs, const NAME& rhs) { return !(lhs == rhs); }


TYPEDEF(entry_index_t, entry_index_type)
TYPEDEF(title_index_t, entry_index_type)
TYPEDEF(cluster_index_t, cluster_index_type)
TYPEDEF(blob_index_t, blob_index_type)

TYPEDEF(zsize_t, size_type)
TYPEDEF(offset_t, offset_type)

#undef TYPEDEF

inline offset_t& operator+= (offset_t& lhs, const zsize_t& rhs)
{
  lhs.v += rhs.v;
  return lhs;
}

inline offset_t operator+(offset_t lhs, const zsize_t& rhs)
{
  lhs += rhs;
  return lhs;
}

};

#endif //ZIM_TYPES_H
