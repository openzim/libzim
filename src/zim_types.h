

#ifndef ZIM_TYPES_H
#define ZIM_TYPES_H

#include <zim/zim.h>

#include <ostream>

template<typename B>
struct REAL_TYPEDEF{
  typedef B base_type;
  B v;
  REAL_TYPEDEF() : v(0) {};
  explicit REAL_TYPEDEF(B v) : v(v) {};
  explicit inline operator bool() const { return v != 0; }
  explicit inline operator B() const { return v; }

  inline bool operator==(const REAL_TYPEDEF<B>& rhs)
  { return v == rhs.v; }
};

template<typename T> inline T& operator+= (T& lhs, const T& rhs)
{
  lhs.v += rhs.v;
  return lhs;
}

template<typename T> inline T& operator+= (T& lhs, const typename T::base_type& rhs)
{
  lhs.v += rhs;
  return lhs;
}

template<typename T> inline T operator+(T lhs, const T& rhs)
{
  lhs += rhs;
  return lhs;
}

template<typename T> inline T& operator-=(T& lhs, const T& rhs)
{
  lhs.v -= rhs.v;
  return lhs;
}

template<typename T> inline T operator-(T lhs, const T& rhs)
{
  lhs -= rhs;
  return lhs;
}

template<typename T> inline bool operator< (const T& lhs, const T& rhs)
{ return lhs.v < rhs.v; }

template<typename T> inline bool operator> (const T& lhs, const T& rhs)
{ return rhs < lhs; }

template<typename T> inline bool operator<=(const T& lhs, const T& rhs)
{ return !(lhs > rhs); }

template<typename T> inline bool operator>=(const T& lhs, const T& rhs)
{ return !(lhs < rhs); }

template<typename T> inline bool operator!=(const T& lhs, const T& rhs)
{ return !(lhs == rhs); }


template<typename B>
std::ostream& operator<<(std::ostream& os, const REAL_TYPEDEF<B>& obj)
{
    os << obj.v;
    return os;
}

namespace zim {

#define TYPEDEF(NAME, TYPE) struct NAME : public REAL_TYPEDEF<TYPE> { \
explicit NAME(TYPE v=0) : REAL_TYPEDEF<TYPE>(v) {}; }; \
static_assert(sizeof(NAME) == sizeof(TYPE), "");

TYPEDEF(article_index_t, article_index_type)
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
