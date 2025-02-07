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

template<typename CostEstimation>
struct FutureToValueCostEstimation {
  template<typename T>
  static size_t cost(const std::shared_future<T>& future) {
    // The future is the value in the cache.
    // When calling getOrPut, if the key is not in the cache,
    // we add a future and then we compute the value and set the future.
    // But lrucache call us when we add the future, meaning before we have
    // computed the value. If we wait here (or use future.get), we will dead lock
    // as we need to exit before setting the value.
    // So in this case, we return 0. `ConcurrentCache::getOrPut` will correctly increase
    // the current cache size when it have an actual value.
    // We still need to compute the size of the value if the future has a value as it
    // is also use to decrease the cache size when the value is drop.
    using namespace std::chrono_literals;
    std::future_status status = future.wait_for(0s);
    if (status == std::future_status::ready) {
      return CostEstimation::cost(future.get());
    } else {
      return 0;
    }
  }

};

/**
   ConcurrentCache implements a concurrent thread-safe cache

   Compared to zim::lru_cache, each access operation is slightly more expensive.
   However, different slots of the cache can be safely accessed concurrently
   with minimal blocking. Concurrent access to the same element is also
   safe, and, in case of a cache miss, will block until that element becomes
   available.
 */
template <typename Key, typename Value, typename CostEstimation>
class ConcurrentCache
{
private: // types
  typedef std::shared_future<Value> ValuePlaceholder;
  typedef lru_cache<Key, ValuePlaceholder, FutureToValueCostEstimation<CostEstimation>> Impl;

public: // types
  explicit ConcurrentCache(size_t maxEntries)
    : impl_(maxEntries)
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
    const auto x = impl_.getOrPut(key, valuePromise.get_future().share());
    l.unlock();
    if ( x.miss() ) {
      try {
        valuePromise.set_value(f());
        {
          std::unique_lock<std::mutex> l(lock_);
          impl_.increaseSize(CostEstimation::cost(x.value().get()));
        }
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
    return impl_.drop(key);
  }

  size_t get_max_size() const {
    std::unique_lock<std::mutex> l(lock_);
    return impl_.get_max_size();
  }

  size_t get_current_size() const {
    std::unique_lock<std::mutex> l(lock_);
    return impl_.size();
  }

  void set_max_size(size_t new_size) {
    std::unique_lock<std::mutex> l(lock_);
    return impl_.set_max_size(new_size);
  }

private: // data
  Impl impl_;
  mutable std::mutex lock_;
};

} // namespace zim

#endif // ZIM_CONCURRENT_CACHE_H

