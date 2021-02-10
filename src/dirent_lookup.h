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

template<class Impl>
class DirentLookup
{
public: // types
  typedef std::pair<bool, entry_index_t> Result;

public: // functions
  DirentLookup(const Impl* _impl, entry_index_type cacheEntryCount);

  entry_index_t getNamespaceRangeBegin(char ns) const;
  entry_index_t getNamespaceRangeEnd(char ns) const;

  Result find(char ns, const std::string& url);

private: // functions
  std::string getDirentKey(entry_index_type i) const;

private: // types
  typedef std::map<char, entry_index_t> NamespaceBoundaryCache;

private: // data
  const Impl* impl = nullptr;

  mutable NamespaceBoundaryCache namespaceBoundaryCache;
  mutable std::mutex cacheAccessMutex;

  entry_index_type direntCount = 0;
  NarrowDown lookupGrid;
};

template<class Impl>
std::string
DirentLookup<Impl>::getDirentKey(entry_index_type i) const
{
  const auto d = impl->getDirent(entry_index_t(i));
  return d->getNamespace() + d->getUrl();
}

template<class Impl>
DirentLookup<Impl>::DirentLookup(const Impl* _impl, entry_index_type cacheEntryCount)
{
  ASSERT(impl == nullptr, ==, true);
  impl = _impl;
  direntCount = entry_index_type(impl->getDirentCount());
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

template<typename IMPL>
entry_index_t getNamespaceBeginOffset(IMPL& impl, char ch)
{
  ASSERT(ch, >=, 32);
  ASSERT(ch, <=, 127);

  entry_index_type lower = 0;
  entry_index_type upper = entry_index_type(impl.getDirentCount());
  auto d = impl.getDirent(entry_index_t(0));
  while (upper - lower > 1)
  {
    entry_index_type m = lower + (upper - lower) / 2;
    auto d = impl.getDirent(entry_index_t(m));
    if (d->getNamespace() >= ch)
      upper = m;
    else
      lower = m;
  }

  entry_index_t ret = entry_index_t(d->getNamespace() < ch ? upper : lower);
  return ret;
}

template<typename IMPL>
entry_index_t getNamespaceEndOffset(IMPL& impl, char ch)
{
  ASSERT(ch, >=, 32);
  ASSERT(ch, <, 127);
  return getNamespaceBeginOffset(impl, ch+1);
}



template<class Impl>
entry_index_t
DirentLookup<Impl>::getNamespaceRangeBegin(char ch) const
{
  ASSERT(ch, >=, 32);
  ASSERT(ch, <=, 127);

  {
    std::lock_guard<std::mutex> lock(cacheAccessMutex);
    const auto it = namespaceBoundaryCache.find(ch);
    if (it != namespaceBoundaryCache.end())
      return it->second;
  }

  auto ret = getNamespaceBeginOffset(*impl, ch);

  std::lock_guard<std::mutex> lock(cacheAccessMutex);
  namespaceBoundaryCache[ch] = ret;
  return ret;
}

template<class Impl>
entry_index_t
DirentLookup<Impl>::getNamespaceRangeEnd(char ns) const
{
  return getNamespaceRangeBegin(ns+1);
}

template<typename Impl>
typename DirentLookup<Impl>::Result
DirentLookup<Impl>::find(char ns, const std::string& url)
{
  const auto r = lookupGrid.getRange(ns + url);
  entry_index_type l(r.begin);
  entry_index_type u(r.end);

  if (l == u)
    return {false, entry_index_t(l)};

  while (true)
  {
    entry_index_type p = l + (u - l) / 2;
    const auto d = impl->getDirent(entry_index_t(p));

    const int c = ns < d->getNamespace() ? -1
                : ns > d->getNamespace() ? 1
                : url.compare(d->getUrl());

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
