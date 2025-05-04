/*
 * Copyrigth (c) 2021, Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (c) 2020, Veloman Yunkan
 * Copyright (c) 2014, lamerman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of lamerman nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * File:   lrucache.hpp
 * Author: Alexander Ponomarev
 *
 * Created on June 20, 2013, 5:09 PM
 */

#ifndef _LRUCACHE_HPP_INCLUDED_
#define _LRUCACHE_HPP_INCLUDED_

#include <map>
#include <list>
#include <cstddef>
#include <stdexcept>
#include <cassert>
#include <iostream>

#include "log.h"

namespace zim {

struct UnitCostEstimation {
  template<typename value_t>
  static size_t cost(const value_t& value) {
    return 1;
  }
};

/**
 * A lru cache where the cost of each item can be different from 1.
 *
 * Most lru caches are limited by the number of items stored.
 * This implementation may have a different "size" per item, so the current
 * size of this cache is not the number of items but the sum of all items' size.
 *
 * The implementation used is pretty simple (dumb) and has a few limitations:
 * - We assume that the size of an item does not change over time. Importantly,
 *   the size of a item when we add it to the cache MUST be equal to the size
 *   of the same item when we drop it from the cache.
 * - Cache eviction still relies on the Least Recently Used (LRU) heuristics,
 *   so we drop the least used item(s) util we have enough space. No other
 *   consideration is used to select which item to drop.
 *
 * This lru cache is parametrized by a CostEstimation type. The type must have a
 * static method `cost` taking a reference to a `value_t` and returing its
 * "cost". As already said, this method must always return the same cost for
 * the same value.
 */
template<typename key_t, typename value_t, typename CostEstimation>
class lru_cache {
public: // types
  typedef typename std::pair<key_t, value_t> key_value_pair_t;
  typedef typename std::list<key_value_pair_t>::iterator list_iterator_t;

  enum AccessStatus {
    HIT, // key was found in the cache
    PUT, // key was not in the cache but was created by the getOrPut() access
    MISS // key was not in the cache; get() access failed
  };

  class AccessResult
  {
    const AccessStatus status_;
    const value_t val_;
  public:
    AccessResult(const value_t& val, AccessStatus status)
      : status_(status), val_(val)
    {}
    AccessResult() : status_(MISS), val_() {}

    bool hit() const { return status_ == HIT; }
    bool miss() const { return !hit(); }
    const value_t& value() const
    {
      if ( status_ == MISS )
        throw std::range_error("There is no such key in cache");
      return val_;
    }

    operator const value_t& () const { return value(); }
  };

public: // functions
  explicit lru_cache(size_t max_cost) :
    _max_cost(max_cost),
    _current_cost(0)
  {}

  // If 'key' is present in the cache, returns the associated value,
  // otherwise puts the given value into the cache (and returns it with
  // a status of a cache miss).
  AccessResult getOrPut(const key_t& key, const value_t& value) {
    log_debug_func_call("lru_cache::getOrPut", key);
    auto it = _cache_items_map.find(key);
    if (it != _cache_items_map.end()) {
      _cache_items_list.splice(_cache_items_list.begin(), _cache_items_list, it->second);
      log_debug("already in cache, moved to the beginning of the LRU list.");
      return AccessResult(it->second->second, HIT);
    } else {
      log_debug("not in cache, adding...");
      putMissing(key, value);
      return AccessResult(value, PUT);
    }
  }

  void put(const key_t& key, const value_t& value) {
    log_debug_func_call("lru_cache::put", key);
    auto it = _cache_items_map.find(key);
    if (it != _cache_items_map.end()) {
      _cache_items_list.splice(_cache_items_list.begin(), _cache_items_list, it->second);
      decreaseCost(CostEstimation::cost(it->second->second));
      increaseCost(CostEstimation::cost(value));
      it->second->second = value;
    } else {
      putMissing(key, value);
    }
  }

  AccessResult get(const key_t& key) {
    auto it = _cache_items_map.find(key);
    if (it == _cache_items_map.end()) {
      return AccessResult();
    } else {
      _cache_items_list.splice(_cache_items_list.begin(), _cache_items_list, it->second);
      return AccessResult(it->second->second, HIT);
    }
  }

  bool drop(const key_t& key) {
    log_debug_func_call("lru_cache::drop", key);
    list_iterator_t list_it;
    try {
      list_it = _cache_items_map.at(key);
    } catch (std::out_of_range& e) {
      log_debug("key not in cache, there is nothing to do");
      return false;
    }
    decreaseCost(CostEstimation::cost(list_it->second));
    _cache_items_list.erase(list_it);
    _cache_items_map.erase(key);
    return true;
  }

  bool exists(const key_t& key) const {
    return _cache_items_map.find(key) != _cache_items_map.end();
  }

  size_t cost() const {
    return _current_cost;
  }

  size_t getMaxCost() const {
    return _max_cost;
  }

  void setMaxCost(size_t newMaxCost) {
    _max_cost = newMaxCost;
    increaseCost(0);
  }

private: // functions
  void increaseCost(size_t extra_cost) {
    log_debug_func_call("lru_cache::increaseCost", extra_cost);
    _current_cost += extra_cost;
    log_debug("_current_cost after increase: " << _current_cost);
    const auto costLimit = std::max(_max_cost, extra_cost);
    while (_current_cost > costLimit) {
      dropLRU();
    }
    log_debug("settled _current_cost: " << _current_cost);
  }

  void decreaseCost(size_t costToRemove) {
    log_debug_func_call("lru_cache::decreaseCost", costToRemove);
    assert(costToRemove <= _current_cost);
    _current_cost -= costToRemove;
    log_debug("_current_cost after decrease: " << _current_cost);
  }

  list_iterator_t getLRUItem() {
    for ( list_iterator_t it = _cache_items_list.end(); it != _cache_items_list.begin(); ) {
      --it;
      if ( CostEstimation::cost(it->second) > 0 )
        return it;
    }
    return _cache_items_list.end();
  }

  void dropLRU() {
    log_debug_func_call("lru_cache::dropLRU");
    const auto lruIt = getLRUItem();
    if ( lruIt == _cache_items_list.end() )
      return;
    const auto key = lruIt->first;
    const auto itemCost = CostEstimation::cost(lruIt->second);
    if ( itemCost > 0 ) {
      log_debug("evicting entry with key: " << key);
      decreaseCost(itemCost);
      _cache_items_map.erase(key);
      _cache_items_list.erase(lruIt);
    }
  }

  void putMissing(const key_t& key, const value_t& value) {
    log_debug_func_call("lru_cache::putMissing", key);
    assert(_cache_items_map.find(key) == _cache_items_map.end());
    _cache_items_list.push_front(key_value_pair_t(key, value));
    _cache_items_map[key] = _cache_items_list.begin();
    increaseCost(CostEstimation::cost(value));
  }

  size_t size() const {
    return _cache_items_map.size();
  }


private: // data
  std::list<key_value_pair_t> _cache_items_list;
  std::map<key_t, list_iterator_t> _cache_items_map;
  size_t _max_cost;
  size_t _current_cost;
};

} // namespace zim

#endif  /* _LRUCACHE_HPP_INCLUDED_ */
