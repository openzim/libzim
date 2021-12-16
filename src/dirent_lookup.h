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

namespace zim
{

template<class TConfig>
class DirentLookup
{
public: // types
  typedef typename TConfig::DirentAccessorType DirentAccessor;
  typedef std::pair<bool, entry_index_t> Result;

public: // functions
  explicit DirentLookup(const DirentAccessor* _direntAccessor);

  entry_index_t getNamespaceRangeBegin(char ns) const;
  entry_index_t getNamespaceRangeEnd(char ns) const;

  Result find(char ns, const std::string& key) const;

protected: // functions
  int compareWithDirentAt(char ns, const std::string& key, entry_index_type i) const;
  Result findInRange(entry_index_type l, entry_index_type u, char ns, const std::string& key) const;

protected: // types
  typedef std::map<char, entry_index_t> NamespaceBoundaryCache;

protected: // data
  const DirentAccessor& direntAccessor;
  const entry_index_type direntCount;

  mutable NamespaceBoundaryCache namespaceBoundaryCache;
  mutable std::mutex cacheAccessMutex;
};

template<class TConfig>
int DirentLookup<TConfig>::compareWithDirentAt(char ns, const std::string& key, entry_index_type i) const
{
  const auto dirent = direntAccessor.getDirent(entry_index_t(i));
  return ns < dirent->getNamespace() ? -1
       : ns > dirent->getNamespace() ? 1
       : key.compare(TConfig::getDirentKey(*dirent));
}

template<class TConfig>
class FastDirentLookup : public DirentLookup<TConfig>
{
  typedef DirentLookup<TConfig> BaseType;
  typedef typename BaseType::DirentAccessor DirentAccessor;

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
  const auto d = direntAccessor.getDirent(entry_index_t(i));
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
entry_index_t
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
entry_index_t
DirentLookup<TConfig>::getNamespaceRangeEnd(char ns) const
{
  return getNamespaceRangeBegin(ns+1);
}

template<typename TConfig>
typename DirentLookup<TConfig>::Result
FastDirentLookup<TConfig>::find(char ns, const std::string& key) const
{
  const auto r = lookupGrid.getRange(ns + key);
  entry_index_type l(r.begin);
  entry_index_type u(r.end);

  if (l == u)
    return {false, entry_index_t(l)};

  return BaseType::findInRange(l, u, ns, key);
}

template<typename TConfig>
typename DirentLookup<TConfig>::Result
DirentLookup<TConfig>::find(char ns, const std::string& key) const
{
  if ( direntCount == 0 )
      return { false, entry_index_t(0) };

  const auto c = compareWithDirentAt(ns, key, 0);
  if ( c < 0 )
      return { false, entry_index_t(0) };
  else if ( c == 0 )
      return { true, entry_index_t(0) };

  if ( compareWithDirentAt(ns, key, direntCount-1) > 0 )
      return { false, entry_index_t(direntCount) };

  return findInRange(0, direntCount, ns, key);
}

template<typename TConfig>
typename DirentLookup<TConfig>::Result
DirentLookup<TConfig>::findInRange(entry_index_type l, entry_index_type u, char ns, const std::string& key) const
{
  while (true)
  {
    entry_index_type p = l + (u - l) / 2;
    const int c = compareWithDirentAt(ns, key, p);

    if (c < 0)
      u = p;
    else if (c > 0)
    {
      if ( l == p )
        return {false, entry_index_t(u)};
      l = p;
    }
    else
      return {true, entry_index_t(p)};
  }
}

} // namespace zim

#endif // ZIM_DIRENT_LOOKUP_H
