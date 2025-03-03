/*
 * Copyright (C) 2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#ifndef ZIM_CONCURRENT_CACHE_H
#define ZIM_CONCURRENT_CACHE_H

#include "lrucache.h"

#include <cstddef>
#include <future>
#include <mutex>

namespace zim
{

/**
   ConcurrentCache implements a concurrent thread-safe cache

   Compared to zim::lru_cache, each access operation is slightly more expensive.
   However, different slots of the cache can be safely accessed concurrently
   with minimal blocking. Concurrent access to the same element is also
   safe, and, in case of a cache miss, will block until that element becomes
   available.
 */
template <typename Key, typename Value>
class ConcurrentCache: private lru_cache<Key, std::shared_future<Value>>
{
private: // types
  typedef std::shared_future<Value> ValuePlaceholder;
  typedef lru_cache<Key, ValuePlaceholder> Impl;

public: // types
  explicit ConcurrentCache(size_t maxEntries)
    : Impl(maxEntries)
  {}

  // Gets the entry corresponding to the given key. If the entry is not in the
  // cache, it is obtained by calling f() (without any arguments) and the
  // result is put into the cache.
  //
  // The cache as a whole is locked only for the duration of accessing
  // the respective slot. If, in the case of the a cache miss, the generation
  // of the missing element takes a long time, only attempts to access that
  // element will block - the rest of the cache remains open to concurrent
  // access.
  template<class F>
  Value getOrPut(const Key& key, F f)
  {
    std::promise<Value> valuePromise;
    std::unique_lock<std::mutex> l(lock_);
    const auto x = Impl::getOrPut(key, valuePromise.get_future().share());
    l.unlock();
    if ( x.miss() ) {
      try {
        valuePromise.set_value(f());
      } catch (std::exception& e) {
        drop(key);
        throw;
      }
    }

    return x.value().get();
  }

  bool drop(const Key& key)
  {
    std::unique_lock<std::mutex> l(lock_);
    return Impl::drop(key);
  }

  size_t getMaxSize() const {
    std::unique_lock<std::mutex> l(lock_);
    return Impl::getMaxSize();
  }

  size_t getCurrentSize() const {
    std::unique_lock<std::mutex> l(lock_);
    return Impl::size();
  }

  void setMaxSize(size_t newSize) {
    std::unique_lock<std::mutex> l(lock_);
    return Impl::setMaxSize(newSize);
  }

private: // data
  mutable std::mutex lock_;
};

} // namespace zim

#endif // ZIM_CONCURRENT_CACHE_H

