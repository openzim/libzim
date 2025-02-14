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
#include <vector>
#include <iostream>

namespace zim {

struct UnitCostEstimation {
  template<typename value_t>
  static size_t cost(const value_t& value) {
    return 1;
  }
};

/**
 * A lru cache where the cost of each item can be different than 1.
 *
 * Most lru cache is limited by the number of items stored.
 * This implementation may have a different "size" per item, so the current size of
 * this lru is not the number of item but the sum of all items' size.
 *
 * This implementation used is pretty simple (dumb) and have few limitations:
 * - We consider than size of a item do not change over time. Especially the size of a
 *   item when we put it MUST be equal to the size of the same item when we drop it.
 * - Cache eviction is still a Least Recently Used (LRU), so we drop the least used item(s) util
 *   we have enough space. No other consideration is used to select which item to drop.
 *
 * This lru is parametrized by a CostEstimation type. The type must have a static method `cost`
 * taking a reference to a `value_t` and returing its "cost". As already said, this method must
 * always return the same cost for the same value.
 *
 * While cost could be any kind of value, this implemention is intended to be used only with
 * `UnitCostEstimation` (classic lru) and `FutureToValueCostEstimation<ClusterMemorySize>`.
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
    auto it = _cache_items_map.find(key);
    if (it != _cache_items_map.end()) {
      _cache_items_list.splice(_cache_items_list.begin(), _cache_items_list, it->second);
      return AccessResult(it->second->second, HIT);
    } else {
      putMissing(key, value);
      return AccessResult(value, PUT);
    }
  }

  void put(const key_t& key, const value_t& value) {
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
    list_iterator_t list_it;
    try {
      list_it = _cache_items_map.at(key);
    } catch (std::out_of_range& e) {
      return false;
    }
    decreaseCost(CostEstimation::cost(list_it->second));
    _cache_items_list.erase(list_it);
    _cache_items_map.erase(key);
    return true;
  }

  template<class F>
  void dropAll(F f) {
    std::vector<key_t> keys_to_drop;
    for (auto key_iter:_cache_items_map) {
      key_t key = key_iter.first;
      if (f(key)) {
        keys_to_drop.push_back(key);
      }
    }

    for(auto key:keys_to_drop) {
      drop(key);
    }
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
    while (newMaxCost < this->cost()) {
      dropLast();
    }
    _max_cost = newMaxCost;
  }

 protected:

  void increaseCost(size_t extra_cost) {
    // increaseSize is called after we have added a value to the cache to update
    // the size of the current cache.
    // We must ensure that we don't drop the value we just added.
    // While it is technically ok to keep no value if max cache size is 0 (or memory size < of the size of one cluster)
    // it will make recreate the value all the time.
    // Let's be nice with our user and be tolerent to misconfiguration.
    if (!extra_cost) {
      // Don't try to remove an item if we have new size == 0.
      // This is the case when concurent cache add a future without value.
      // We will handle the real increase size when concurent cache will directly call us.
      return;
    }
    _current_cost += extra_cost;
    while (_current_cost > _max_cost && size() > 1) {
      dropLast();
    }
  }

  void decreaseCost(size_t costToRemove) {
    if (costToRemove > _current_cost) {
      std::cerr << "WARNING: We have detected inconsistant cache management, trying to remove " << costToRemove << " from a cache with size " << _current_cost << std::endl;
      std::cerr << "Please open an issue on https://github.com/openzim/libzim/issues with this message and the zim file you use" << std::endl;
      _current_cost = 0;
    } else {
      _current_cost -= costToRemove;
    }
  }

private: // functions
  void dropLast() {
    auto list_it = _cache_items_list.back();
    decreaseCost(CostEstimation::cost(list_it.second));
    _cache_items_map.erase(_cache_items_list.back().first);
    _cache_items_list.pop_back();
  }

  void putMissing(const key_t& key, const value_t& value) {
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
