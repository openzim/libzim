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
#include "log.h"

#include <chrono>
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
template <typename Key, typename Value, typename CostEstimation>
class ConcurrentCache
{
private: // types
  typedef std::shared_future<Value> ValuePlaceholder;

  struct CacheEntry
  {
    size_t            cost = 0;
    ValuePlaceholder  value;

    bool ready() const {
      const auto zeroNs = std::chrono::nanoseconds::zero();
      return value.wait_for(zeroNs) == std::future_status::ready;
    }
  };

  struct GetCacheEntryCost
  {
    static size_t cost(const CacheEntry& x) { return x.cost; }
  };

  typedef lru_cache<Key, CacheEntry, GetCacheEntryCost> Impl;

public: // functions
  explicit ConcurrentCache(size_t maxCost)
    : impl_(maxCost)
  {}

  // Gets the entry corresponding to the given key. If the entry is not in the
  // cache, it is obtained by calling f() (without any arguments) and the
  // result is put into the cache.
  //
  // The cache as a whole is locked only for the duration of accessing
  // the respective slot. If, in the case of a cache miss, the generation
  // of the missing element takes a long time, only attempts to access that
  // element will block - the rest of the cache remains open to concurrent
  // access.
  template<class F>
  Value getOrPut(const Key& key, F f)
  {
    log_debug_func_call("ConcurrentCache::getOrPut", key);

    std::promise<Value> valuePromise;
    const auto x = getCacheSlot(key, valuePromise.get_future().share());
    CacheEntry cacheEntry(x.value());
    log_debug("Obtained the cache slot");
    if ( x.miss() ) {
      log_debug("It was a cache miss. Going to obtain the value...");
      try {
        cacheEntry.cost = materializeValue(valuePromise, f);
        finalizeCacheMiss(key, cacheEntry);
        log_debug("Done. Cache cost is at " << getCurrentCost() );
      } catch (std::exception& e) {
        log_debug("Evaluation failed. Releasing the cache slot...");
        drop(key);
        throw;
      }
    }

    log_debug((!cacheEntry.ready() ? "Waiting for result..." : "Returning immediately..."));
    return log_debug_return_value(cacheEntry.value.get());
  }

  bool drop(const Key& key)
  {
    log_debug_func_call("ConcurrentCache::drop", key);
    log_debug_raii_sync_statement(std::unique_lock<std::mutex> l(lock_));
    return impl_.drop(key);
  }

  size_t getMaxCost() const {
    std::unique_lock<std::mutex> l(lock_);
    return impl_.getMaxCost();
  }

  size_t getCurrentCost() const {
    std::unique_lock<std::mutex> l(lock_);
    return impl_.cost();
  }

  void setMaxCost(size_t newSize) {
    log_debug_func_call("ConcurrentCache::setMaxCost", newSize);
    log_debug_raii_sync_statement(std::unique_lock<std::mutex> l(lock_));
    return impl_.setMaxCost(newSize);
  }

private: // functions
  typename Impl::AccessResult getCacheSlot(const Key& key, const ValuePlaceholder& v)
  {
    log_debug_func_call("ConcurrentCache::getCacheSlot", key);
    log_debug_raii_sync_statement(std::unique_lock<std::mutex> l(lock_));
    return impl_.getOrPut(key, CacheEntry{0, v});
  }

  template<class F>
  static size_t materializeValue(std::promise<Value>& valuePromise, F f)
  {
    const auto materializedValue = f();
    log_debug("Value was successfully obtained.");
    valuePromise.set_value(materializedValue);
    log_debug("Made the value available for concurrent access.");
    log_debug("Computing the cost of the new entry...");
    auto cost = CostEstimation::cost(materializedValue);
    log_debug("cost=" << cost);
    return cost;
  }

  void finalizeCacheMiss(const Key& key, const CacheEntry& cacheEntry)
  {
    log_debug_func_call("ConcurrentCache::finalizeCacheMiss", key);
    log_debug_raii_sync_statement(std::unique_lock<std::mutex> l(lock_));
    impl_.put(key, cacheEntry);
  }

private: // data
  Impl impl_;
  mutable std::mutex lock_;
};

} // namespace zim

#endif // ZIM_CONCURRENT_CACHE_H

