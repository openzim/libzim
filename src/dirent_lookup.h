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

#ifndef ZIM_DIRENT_LOOKUP_H
#define ZIM_DIRENT_LOOKUP_H

#include "zim_types.h"
#include "debug.h"
#include "narrowdown.h"

#include <algorithm>
#include <map>
#include <mutex>
#include <vector>
#include <cassert>

namespace zim
{

template<class TConfig>
class DirentLookup
{
public: // types
  typedef typename TConfig::DirentAccessorType DirentAccessor;
  typedef typename TConfig::index_t index_t;
  typedef std::pair<bool, index_t> Result;

public: // functions
  explicit DirentLookup(const DirentAccessor* _direntAccessor);

  index_t getNamespaceRangeBegin(char ns) const;
  index_t getNamespaceRangeEnd(char ns) const;

  Result find(char ns, const std::string& key) const;

protected: // functions
  int compareWithDirentAt(char ns, const std::string& key, entry_index_type i) const;
  Result findInRange(entry_index_type l, entry_index_type u, char ns, const std::string& key) const;
  Result binarySearchInRange(entry_index_type l, entry_index_type u, char ns, const std::string& key) const;

protected: // types
  typedef std::map<char, index_t> NamespaceBoundaryCache;

protected: // data
  const DirentAccessor& direntAccessor;
  const entry_index_type direntCount;

  mutable NamespaceBoundaryCache namespaceBoundaryCache;
  mutable std::mutex cacheAccessMutex;
};

template<class TConfig>
int DirentLookup<TConfig>::compareWithDirentAt(char ns, const std::string& key, entry_index_type i) const
{
  const auto dirent = direntAccessor.getDirent(index_t(i));
  return ns < dirent->getNamespace() ? -1
       : ns > dirent->getNamespace() ? 1
       : key.compare(TConfig::getDirentKey(*dirent));
}

template<class TConfig>
class FastDirentLookup : public DirentLookup<TConfig>
{
  typedef DirentLookup<TConfig> BaseType;
  typedef typename BaseType::DirentAccessor DirentAccessor;
  typedef typename BaseType::index_t index_t;

public: // functions
  FastDirentLookup(const DirentAccessor* _direntAccessor, entry_index_type cacheEntryCount);

  typename BaseType::Result find(char ns, const std::string& key) const;

private: // functions
  std::string getDirentKey(entry_index_type i) const;

private: // data
  using BaseType::direntAccessor;
  using BaseType::direntCount;
  NarrowDown lookupGrid;
};

template<class TConfig>
std::string
FastDirentLookup<TConfig>::getDirentKey(entry_index_type i) const
{
  const auto d = direntAccessor.getDirent(index_t(i));
  return d->getNamespace() + TConfig::getDirentKey(*d);
}

template<class TConfig>
DirentLookup<TConfig>::DirentLookup(const DirentAccessor* _direntAccessor)
  : direntAccessor(*_direntAccessor)
  , direntCount(direntAccessor.getDirentCount())
{
}

template<class TConfig>
FastDirentLookup<TConfig>::FastDirentLookup(const DirentAccessor* _direntAccessor, entry_index_type cacheEntryCount)
  : BaseType(_direntAccessor)
{
  if ( direntCount )
  {
    const entry_index_type step = std::max(1u, direntCount/cacheEntryCount);
    for ( entry_index_type i = 0; i < direntCount-1; i += step )
    {
        lookupGrid.add(getDirentKey(i), i, getDirentKey(i+1));
    }
    lookupGrid.close(getDirentKey(direntCount - 1), direntCount - 1);
  }
}

template<typename TDirentAccessor>
entry_index_t getNamespaceBeginOffset(TDirentAccessor& direntAccessor, char ch)
{
  ASSERT(ch, >=, 32);
  ASSERT(ch, <=, 127);

  entry_index_type lower = 0;
  entry_index_type upper = entry_index_type(direntAccessor.getDirentCount());
  auto d = direntAccessor.getDirent(entry_index_t(0));
  while (upper - lower > 1)
  {
    entry_index_type m = lower + (upper - lower) / 2;
    auto d = direntAccessor.getDirent(entry_index_t(m));
    if (d->getNamespace() >= ch)
      upper = m;
    else
      lower = m;
  }

  entry_index_t ret = entry_index_t(d->getNamespace() < ch ? upper : lower);
  return ret;
}

template<typename TDirentAccessor>
entry_index_t getNamespaceEndOffset(TDirentAccessor& direntAccessor, char ch)
{
  ASSERT(ch, >=, 32);
  ASSERT(ch, <, 127);
  return getNamespaceBeginOffset(direntAccessor, ch+1);
}



template<class TConfig>
typename DirentLookup<TConfig>::index_t
DirentLookup<TConfig>::getNamespaceRangeBegin(char ch) const
{
  ASSERT(ch, >=, 32);
  ASSERT(ch, <=, 127);

  {
    std::lock_guard<std::mutex> lock(cacheAccessMutex);
    const auto it = namespaceBoundaryCache.find(ch);
    if (it != namespaceBoundaryCache.end())
      return it->second;
  }

  auto ret = getNamespaceBeginOffset(direntAccessor, ch);

  std::lock_guard<std::mutex> lock(cacheAccessMutex);
  namespaceBoundaryCache[ch] = ret;
  return ret;
}

template<class TConfig>
typename DirentLookup<TConfig>::index_t
DirentLookup<TConfig>::getNamespaceRangeEnd(char ns) const
{
  return getNamespaceRangeBegin(ns+1);
}

template<typename TConfig>
typename DirentLookup<TConfig>::Result
FastDirentLookup<TConfig>::find(char ns, const std::string& key) const
{
  const auto r = lookupGrid.getRange(ns + key);
  return BaseType::findInRange(r.begin, r.end, ns, key);
}

template<typename TConfig>
typename DirentLookup<TConfig>::Result
DirentLookup<TConfig>::find(char ns, const std::string& key) const
{
  return findInRange(0, direntCount, ns, key);
}

template<typename TConfig>
typename DirentLookup<TConfig>::Result
DirentLookup<TConfig>::findInRange(entry_index_type l, entry_index_type u, char ns, const std::string& key) const
{
  if ( l == u )
      return { false, index_t(l) };

  const auto c = compareWithDirentAt(ns, key, l);
  if ( c < 0 )
      return { false, index_t(l) };
  else if ( c == 0 )
      return { true, index_t(l) };

  if ( compareWithDirentAt(ns, key, u-1) > 0 )
      return { false, index_t(u) };

  return binarySearchInRange(l, u-1, ns, key);
}

template<typename TConfig>
typename DirentLookup<TConfig>::Result
DirentLookup<TConfig>::binarySearchInRange(entry_index_type l, entry_index_type u, char ns, const std::string& key) const
{
  assert(l <= u && u < direntCount);
  assert(compareWithDirentAt(ns, key, l) > 0);
  assert(compareWithDirentAt(ns, key, u) <= 0);
  // Invariant maintained by the binary search:
  //    (entry at l) < (query entry ns/key) <= (entry at u)
  while (true)
  {
    // compute p as the **upward rounded** average of l and u
    const entry_index_type p = l + (u - l + 1) / 2;
    const int c = compareWithDirentAt(ns, key, p);
    if (c <= 0) { // (entry at l) < ns/key <= (entry at p) <= (entry at u)
      if ( u == p ) {
        return { c == 0, index_t(u) };
      }
      u = p;
    } else {  // (entry at l) < (entry at p) < ns/key <= (entry at u)
      l = p;
    }
  }
}

} // namespace zim

#endif // ZIM_DIRENT_LOOKUP_H
