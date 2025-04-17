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

#include <chrono>
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
    std::future_status status = future.wait_for(std::chrono::nanoseconds::zero());
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
class ConcurrentCache: private lru_cache<Key, std::shared_future<Value>, FutureToValueCostEstimation<CostEstimation>>
{
private: // types
  typedef std::shared_future<Value> ValuePlaceholder;
  typedef lru_cache<Key, ValuePlaceholder, FutureToValueCostEstimation<CostEstimation>> Impl;

public: // types
  explicit ConcurrentCache(size_t maxCost)
    : Impl(maxCost)
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
    auto shared_future = valuePromise.get_future().share();
    const auto x = Impl::getOrPut(key, shared_future);
    l.unlock();
    if ( x.miss() ) {
      try {
        valuePromise.set_value(f());
        auto cost = CostEstimation::cost(x.value().get());
        // There is a small window when the valuePromise may be drop from lru cache after
        // we set the value but before we increase the size of the cache.
        // In this case we decrease the size of `cost` before increasing it.
        // First of all it should be pretty rare as we have just put the future in the cache so it
        // should not be the least used item.
        // If it happens, this should not be a problem if current_size is bigger than `cost` (most of the time)
        // For the really rare specific case of current cach size being lower than `cost` (if possible),
        // `decreaseCost` will clamp the new size to 0.
        {
          std::unique_lock<std::mutex> l(lock_);
          // There is a window when the shared_future is drop from the cache while we are computing the value.
          // If this is the case, we readd the shared_future in the cache.
          if (!Impl::exists(key)) {
            // We don't have have to increase the cache as the future is already set, so the cost will be valid.
            Impl::put(key, shared_future);
          } else {
            // We just have to increase the cost as we used 0 for unset future.
            Impl::increaseCost(cost);
          }
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
    return Impl::drop(key);
  }

  template<class F>
  void dropAll(F f) {
    std::unique_lock<std::mutex> l(lock_);
    Impl::dropAll(f);
  }

  size_t getMaxCost() const {
    std::unique_lock<std::mutex> l(lock_);
    return Impl::getMaxCost();
  }

  size_t getCurrentCost() const {
    std::unique_lock<std::mutex> l(lock_);
    return Impl::cost();
  }

  void setMaxCost(size_t newSize) {
    std::unique_lock<std::mutex> l(lock_);
    return Impl::setMaxCost(newSize);
  }

private: // data
  mutable std::mutex lock_;
};

} // namespace zim

#endif // ZIM_CONCURRENT_CACHE_H

