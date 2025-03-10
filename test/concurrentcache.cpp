/*
 * Copyright (C) 2025 Veloman Yunkan
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

#define LIBZIM_ENABLE_LOGGING

#include "concurrent_cache.h"
#include "gtest/gtest.h"

struct LazyValue
{
    const int value;
    explicit LazyValue(int v) : value(v) {}
    int operator()() const { return value; }
};

struct ExceptionSource
{
    int operator()() const { throw std::runtime_error("oops"); return 0; }
};

TEST(ConcurrentCacheTest, handleException) {
    zim::ConcurrentCache<int, int, zim::UnitCostEstimation> cache(1);
    EXPECT_EQ(cache.getOrPut(7, LazyValue(777)), 777);
    EXPECT_THROW(cache.getOrPut(8, ExceptionSource()), std::runtime_error);
    EXPECT_EQ(cache.getOrPut(8, LazyValue(888)), 888);
}

TEST(ConcurrentCacheXRayTest, simpleFlow) {
    zim::Logging::logIntoMemory();
    zim::ConcurrentCache<int, int, zim::UnitCostEstimation> cache(1);
    EXPECT_EQ(cache.getOrPut(3, LazyValue(2025)), 2025);
    EXPECT_EQ(cache.getOrPut(3, LazyValue(123)),  2025);
    EXPECT_THROW(cache.getOrPut(2, ExceptionSource()), std::runtime_error);
    EXPECT_EQ(cache.getOrPut(2, LazyValue(123)),  123);

    ASSERT_EQ(zim::Logging::getInMemLogContent(),
R"(thread#0: ConcurrentCache::getOrPut(3) {
thread#0:  lru_cache::getOrPut(3) {
thread#0:   not in cache, adding...
thread#0:   lru_cache::increaseCost(0) {
thread#0:   }
thread#0:  }
thread#0:  Obtained the cache slot
thread#0:  It was a cache miss. Going to obtain the value...
thread#0:  Value was successfully obtained. Computing its cost...
thread#0:  cost=1. Committing to cache...
thread#0:  lru_cache::increaseCost(1) {
thread#0:   _current_cost after increase: 1
thread#0:   settled _current_cost: 1
thread#0:  }
thread#0:  Done. Cache cost is at 1
thread#0: } (return value: 2025)
thread#0: ConcurrentCache::getOrPut(3) {
thread#0:  lru_cache::getOrPut(3) {
thread#0:   already in cache, moved to the beginning of the LRU list.
thread#0:  }
thread#0:  Obtained the cache slot
thread#0: } (return value: 2025)
thread#0: ConcurrentCache::getOrPut(2) {
thread#0:  lru_cache::getOrPut(2) {
thread#0:   not in cache, adding...
thread#0:   lru_cache::increaseCost(0) {
thread#0:   }
thread#0:  }
thread#0:  Obtained the cache slot
thread#0:  It was a cache miss. Going to obtain the value...
thread#0:  Evaluation failed. Releasing the cache slot...
thread#0:  ConcurrentCache::drop(2) {
thread#0:   lru_cache::drop(2) {
thread#0:    lru_cache::decreaseCost(0) {
thread#0:     _current_cost after decrease: 1
thread#0:    }
thread#0:   }
thread#0:  }
thread#0: }
thread#0: ConcurrentCache::getOrPut(2) {
thread#0:  lru_cache::getOrPut(2) {
thread#0:   not in cache, adding...
thread#0:   lru_cache::increaseCost(0) {
thread#0:   }
thread#0:  }
thread#0:  Obtained the cache slot
thread#0:  It was a cache miss. Going to obtain the value...
thread#0:  Value was successfully obtained. Computing its cost...
thread#0:  cost=1. Committing to cache...
thread#0:  lru_cache::increaseCost(1) {
thread#0:   _current_cost after increase: 2
thread#0:   lru_cache::dropLast() {
thread#0:    evicting entry with key: 3
thread#0:    lru_cache::decreaseCost(1) {
thread#0:     _current_cost after decrease: 1
thread#0:    }
thread#0:   }
thread#0:   settled _current_cost: 1
thread#0:  }
thread#0:  Done. Cache cost is at 1
thread#0: } (return value: 123)
)");
}
