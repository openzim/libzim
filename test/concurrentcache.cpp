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

#include "namedthread.h"

struct LazyValue
{
    typedef std::chrono::milliseconds Milliseconds;

    const int value;
    const Milliseconds delay;

    explicit LazyValue(int v, Milliseconds evaluationDelay = Milliseconds(0))
      : value(v)
      , delay(evaluationDelay)
    {}

    int operator()() const
    {
      std::this_thread::sleep_for(delay);
      return value;
    }
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

TEST(ConcurrentCacheTest, addAnItemToAnEmptyCache) {
    zim::ConcurrentCache<int, int, zim::UnitCostEstimation> cache(1);

    zim::Logging::logIntoMemory();
    EXPECT_EQ(cache.getOrPut(3, LazyValue(2025)), 2025);
    ASSERT_EQ(zim::Logging::getInMemLogContent(),
R"(thread#0: ConcurrentCache::getOrPut(3) {
thread#0:  ConcurrentCache::getCacheSlot(3) {
thread#0:   entered synchronized section
thread#0:   lru_cache::getOrPut(3) {
thread#0:    not in cache, adding...
thread#0:    lru_cache::putMissing(3) {
thread#0:     lru_cache::increaseCost(0) {
thread#0:      _current_cost after increase: 0
thread#0:      settled _current_cost: 0
thread#0:     }
thread#0:    }
thread#0:   }
thread#0:   exiting synchronized section
thread#0:  }
thread#0:  Obtained the cache slot
thread#0:  It was a cache miss. Going to obtain the value...
thread#0:  Value was successfully obtained.
thread#0:  Made the value available for concurrent access.
thread#0:  Computing the cost of the new entry...
thread#0:  cost=1
thread#0:  ConcurrentCache::finalizeCacheMiss(3) {
thread#0:   entered synchronized section
thread#0:   lru_cache::put(3) {
thread#0:    lru_cache::decreaseCost(0) {
thread#0:     _current_cost after decrease: 0
thread#0:    }
thread#0:    lru_cache::increaseCost(1) {
thread#0:     _current_cost after increase: 1
thread#0:     settled _current_cost: 1
thread#0:    }
thread#0:   }
thread#0:   exiting synchronized section
thread#0:  }
thread#0:  Done. Cache cost is at 1
thread#0:  Returning immediately...
thread#0: } (return value: 2025)
)");
}

TEST(ConcurrentCacheTest, cacheHit) {
    zim::ConcurrentCache<int, int, zim::UnitCostEstimation> cache(1);
    cache.getOrPut(3, LazyValue(2025));

    zim::Logging::logIntoMemory();
    EXPECT_EQ(cache.getOrPut(3, LazyValue(123)),  2025);
    ASSERT_EQ(zim::Logging::getInMemLogContent(),
R"(thread#0: ConcurrentCache::getOrPut(3) {
thread#0:  ConcurrentCache::getCacheSlot(3) {
thread#0:   entered synchronized section
thread#0:   lru_cache::getOrPut(3) {
thread#0:    already in cache, moved to the beginning of the LRU list.
thread#0:   }
thread#0:   exiting synchronized section
thread#0:  }
thread#0:  Obtained the cache slot
thread#0:  Returning immediately...
thread#0: } (return value: 2025)
)");
}

TEST(ConcurrentCacheTest, attemptToAddNonMaterializableItemToFullCache) {
    zim::ConcurrentCache<int, int, zim::UnitCostEstimation> cache(1);
    cache.getOrPut(3, LazyValue(2025));

    zim::Logging::logIntoMemory();
    EXPECT_THROW(cache.getOrPut(2, ExceptionSource()), std::runtime_error);
    ASSERT_EQ(zim::Logging::getInMemLogContent(),
R"(thread#0: ConcurrentCache::getOrPut(2) {
thread#0:  ConcurrentCache::getCacheSlot(2) {
thread#0:   entered synchronized section
thread#0:   lru_cache::getOrPut(2) {
thread#0:    not in cache, adding...
thread#0:    lru_cache::putMissing(2) {
thread#0:     lru_cache::increaseCost(0) {
thread#0:      _current_cost after increase: 1
thread#0:      settled _current_cost: 1
thread#0:     }
thread#0:    }
thread#0:   }
thread#0:   exiting synchronized section
thread#0:  }
thread#0:  Obtained the cache slot
thread#0:  It was a cache miss. Going to obtain the value...
thread#0:  Evaluation failed. Releasing the cache slot...
thread#0:  ConcurrentCache::drop(2) {
thread#0:   entered synchronized section
thread#0:   lru_cache::drop(2) {
thread#0:    lru_cache::decreaseCost(0) {
thread#0:     _current_cost after decrease: 1
thread#0:    }
thread#0:   }
thread#0:   exiting synchronized section
thread#0:  }
thread#0: }
)");
}

TEST(ConcurrentCacheTest, addItemToFullCache) {
    zim::ConcurrentCache<int, int, zim::UnitCostEstimation> cache(1);
    cache.getOrPut(3, LazyValue(2025));

    zim::Logging::logIntoMemory();
    EXPECT_EQ(cache.getOrPut(2, LazyValue(123)),  123);
    ASSERT_EQ(zim::Logging::getInMemLogContent(),
R"(thread#0: ConcurrentCache::getOrPut(2) {
thread#0:  ConcurrentCache::getCacheSlot(2) {
thread#0:   entered synchronized section
thread#0:   lru_cache::getOrPut(2) {
thread#0:    not in cache, adding...
thread#0:    lru_cache::putMissing(2) {
thread#0:     lru_cache::increaseCost(0) {
thread#0:      _current_cost after increase: 1
thread#0:      settled _current_cost: 1
thread#0:     }
thread#0:    }
thread#0:   }
thread#0:   exiting synchronized section
thread#0:  }
thread#0:  Obtained the cache slot
thread#0:  It was a cache miss. Going to obtain the value...
thread#0:  Value was successfully obtained.
thread#0:  Made the value available for concurrent access.
thread#0:  Computing the cost of the new entry...
thread#0:  cost=1
thread#0:  ConcurrentCache::finalizeCacheMiss(2) {
thread#0:   entered synchronized section
thread#0:   lru_cache::put(2) {
thread#0:    lru_cache::decreaseCost(0) {
thread#0:     _current_cost after decrease: 1
thread#0:    }
thread#0:    lru_cache::increaseCost(1) {
thread#0:     _current_cost after increase: 2
thread#0:     lru_cache::dropLast() {
thread#0:      evicting entry with key: 3
thread#0:      lru_cache::decreaseCost(1) {
thread#0:       _current_cost after decrease: 1
thread#0:      }
thread#0:     }
thread#0:     settled _current_cost: 1
thread#0:    }
thread#0:   }
thread#0:   exiting synchronized section
thread#0:  }
thread#0:  Done. Cache cost is at 1
thread#0:  Returning immediately...
thread#0: } (return value: 123)
)");
}

struct CostAs3xValue
{
  static size_t cost(size_t v) { return 3 * v; }
};

TEST(ConcurrentCacheTest, addOversizedItemToEmptyCache) {
    zim::ConcurrentCache<int, size_t, CostAs3xValue> cache(1000);

    zim::Logging::logIntoMemory();
    cache.getOrPut(151, LazyValue(2025));
    ASSERT_EQ(zim::Logging::getInMemLogContent(),
R"(thread#0: ConcurrentCache::getOrPut(151) {
thread#0:  ConcurrentCache::getCacheSlot(151) {
thread#0:   entered synchronized section
thread#0:   lru_cache::getOrPut(151) {
thread#0:    not in cache, adding...
thread#0:    lru_cache::putMissing(151) {
thread#0:     lru_cache::increaseCost(0) {
thread#0:      _current_cost after increase: 0
thread#0:      settled _current_cost: 0
thread#0:     }
thread#0:    }
thread#0:   }
thread#0:   exiting synchronized section
thread#0:  }
thread#0:  Obtained the cache slot
thread#0:  It was a cache miss. Going to obtain the value...
thread#0:  Value was successfully obtained.
thread#0:  Made the value available for concurrent access.
thread#0:  Computing the cost of the new entry...
thread#0:  cost=6075
thread#0:  ConcurrentCache::finalizeCacheMiss(151) {
thread#0:   entered synchronized section
thread#0:   lru_cache::put(151) {
thread#0:    lru_cache::decreaseCost(0) {
thread#0:     _current_cost after decrease: 0
thread#0:    }
thread#0:    lru_cache::increaseCost(6075) {
thread#0:     _current_cost after increase: 6075
thread#0:     settled _current_cost: 6075
thread#0:    }
thread#0:   }
thread#0:   exiting synchronized section
thread#0:  }
thread#0:  Done. Cache cost is at 6075
thread#0:  Returning immediately...
thread#0: } (return value: 2025)
)");
}

template<class CacheType>
void populateCache(CacheType& c, const std::vector<std::pair<int, int>>& kvs) {
  for ( const auto& kv : kvs ) {
    c.getOrPut(kv.first, LazyValue(kv.second));
  }
}

TEST(ConcurrentCacheTest, addItemsToEmptyCacheWithoutOverflowingIt) {
    zim::ConcurrentCache<int, size_t, CostAs3xValue> cache(1000);

    zim::Logging::logIntoMemory();
    populateCache(cache, { {22, 100}, {11, 200} } );
    ASSERT_EQ(zim::Logging::getInMemLogContent(),
R"(thread#0: ConcurrentCache::getOrPut(22) {
thread#0:  ConcurrentCache::getCacheSlot(22) {
thread#0:   entered synchronized section
thread#0:   lru_cache::getOrPut(22) {
thread#0:    not in cache, adding...
thread#0:    lru_cache::putMissing(22) {
thread#0:     lru_cache::increaseCost(0) {
thread#0:      _current_cost after increase: 0
thread#0:      settled _current_cost: 0
thread#0:     }
thread#0:    }
thread#0:   }
thread#0:   exiting synchronized section
thread#0:  }
thread#0:  Obtained the cache slot
thread#0:  It was a cache miss. Going to obtain the value...
thread#0:  Value was successfully obtained.
thread#0:  Made the value available for concurrent access.
thread#0:  Computing the cost of the new entry...
thread#0:  cost=300
thread#0:  ConcurrentCache::finalizeCacheMiss(22) {
thread#0:   entered synchronized section
thread#0:   lru_cache::put(22) {
thread#0:    lru_cache::decreaseCost(0) {
thread#0:     _current_cost after decrease: 0
thread#0:    }
thread#0:    lru_cache::increaseCost(300) {
thread#0:     _current_cost after increase: 300
thread#0:     settled _current_cost: 300
thread#0:    }
thread#0:   }
thread#0:   exiting synchronized section
thread#0:  }
thread#0:  Done. Cache cost is at 300
thread#0:  Returning immediately...
thread#0: } (return value: 100)
thread#0: ConcurrentCache::getOrPut(11) {
thread#0:  ConcurrentCache::getCacheSlot(11) {
thread#0:   entered synchronized section
thread#0:   lru_cache::getOrPut(11) {
thread#0:    not in cache, adding...
thread#0:    lru_cache::putMissing(11) {
thread#0:     lru_cache::increaseCost(0) {
thread#0:      _current_cost after increase: 300
thread#0:      settled _current_cost: 300
thread#0:     }
thread#0:    }
thread#0:   }
thread#0:   exiting synchronized section
thread#0:  }
thread#0:  Obtained the cache slot
thread#0:  It was a cache miss. Going to obtain the value...
thread#0:  Value was successfully obtained.
thread#0:  Made the value available for concurrent access.
thread#0:  Computing the cost of the new entry...
thread#0:  cost=600
thread#0:  ConcurrentCache::finalizeCacheMiss(11) {
thread#0:   entered synchronized section
thread#0:   lru_cache::put(11) {
thread#0:    lru_cache::decreaseCost(0) {
thread#0:     _current_cost after decrease: 300
thread#0:    }
thread#0:    lru_cache::increaseCost(600) {
thread#0:     _current_cost after increase: 900
thread#0:     settled _current_cost: 900
thread#0:    }
thread#0:   }
thread#0:   exiting synchronized section
thread#0:  }
thread#0:  Done. Cache cost is at 900
thread#0:  Returning immediately...
thread#0: } (return value: 200)
)");
}

TEST(ConcurrentCacheTest, reduceCacheCostLimitBelowCurrentCostValue) {
    zim::ConcurrentCache<int, size_t, CostAs3xValue> cache(1000);

    populateCache(cache, { {5, 50}, {10, 100}, {15, 150} } );

    zim::Logging::logIntoMemory();
    cache.setMaxCost(500);
    ASSERT_EQ(cache.getCurrentCost(), 450);
    ASSERT_EQ(zim::Logging::getInMemLogContent(),
R"(thread#0: ConcurrentCache::setMaxCost(500) {
thread#0:  entered synchronized section
thread#0:  lru_cache::increaseCost(0) {
thread#0:   _current_cost after increase: 900
thread#0:   lru_cache::dropLast() {
thread#0:    evicting entry with key: 5
thread#0:    lru_cache::decreaseCost(150) {
thread#0:     _current_cost after decrease: 750
thread#0:    }
thread#0:   }
thread#0:   lru_cache::dropLast() {
thread#0:    evicting entry with key: 10
thread#0:    lru_cache::decreaseCost(300) {
thread#0:     _current_cost after decrease: 450
thread#0:    }
thread#0:   }
thread#0:   settled _current_cost: 450
thread#0:  }
thread#0:  exiting synchronized section
thread#0: }
)");
}

TEST(ConcurrentCacheTest, reduceCacheCostLimitBelowCostOfMRUItem) {
    zim::ConcurrentCache<int, size_t, CostAs3xValue> cache(1000);

    populateCache(cache, { {5, 50}, {10, 100}, {15, 150} } );

    zim::Logging::logIntoMemory();
    cache.setMaxCost(400);
    ASSERT_EQ(cache.getCurrentCost(), 0);
    ASSERT_EQ(zim::Logging::getInMemLogContent(),
R"(thread#0: ConcurrentCache::setMaxCost(400) {
thread#0:  entered synchronized section
thread#0:  lru_cache::increaseCost(0) {
thread#0:   _current_cost after increase: 900
thread#0:   lru_cache::dropLast() {
thread#0:    evicting entry with key: 5
thread#0:    lru_cache::decreaseCost(150) {
thread#0:     _current_cost after decrease: 750
thread#0:    }
thread#0:   }
thread#0:   lru_cache::dropLast() {
thread#0:    evicting entry with key: 10
thread#0:    lru_cache::decreaseCost(300) {
thread#0:     _current_cost after decrease: 450
thread#0:    }
thread#0:   }
thread#0:   lru_cache::dropLast() {
thread#0:    evicting entry with key: 15
thread#0:    lru_cache::decreaseCost(450) {
thread#0:     _current_cost after decrease: 0
thread#0:    }
thread#0:   }
thread#0:   settled _current_cost: 0
thread#0:  }
thread#0:  exiting synchronized section
thread#0: }
)");
}

TEST(ConcurrentCacheMultithreadedTest, concurrentCacheHit) {
    zim::ConcurrentCache<int, size_t, CostAs3xValue> cache(1000);

    populateCache(cache, { {5, 50}, {10, 100}, {15, 150} } );

    const std::string targetOutput =
R"(thread#0: Output of interest starts from the next line
a  : ConcurrentCache::getOrPut(5) {
  b: ConcurrentCache::getOrPut(5) {
a  :  ConcurrentCache::getCacheSlot(5) {
a  :   entered synchronized section
  b:  ConcurrentCache::getCacheSlot(5) {
a  :   lru_cache::getOrPut(5) {
a  :    already in cache, moved to the beginning of the LRU list.
a  :   }
a  :   exiting synchronized section
a  :  }
  b:   entered synchronized section
a  :  Obtained the cache slot
  b:   lru_cache::getOrPut(5) {
a  :  Returning immediately...
a  : } (return value: 50)
  b:    already in cache, moved to the beginning of the LRU list.
  b:   }
  b:   exiting synchronized section
  b:  }
  b:  Obtained the cache slot
  b:  Returning immediately...
  b: } (return value: 50)
)";

    zim::Logging::logIntoMemory();
    zim::Logging::orchestrateConcurrentExecutionVia(targetOutput);

    const auto accessKey5 = [&cache]() { cache.getOrPut(5, LazyValue(0)); };
    log_debug("Output of interest starts from the next line");
    zim::NamedThread thread1("a  ", accessKey5 );
    zim::NamedThread thread2("  b", accessKey5 );
    thread1.join();
    thread2.join();

    ASSERT_EQ(zim::Logging::getInMemLogContent(), targetOutput);
}

TEST(ConcurrentCacheMultithreadedTest, concurrentCacheMissWithoutEviction) {
    // This test checks that during a concurrent cache miss access
    // 1. only one of the threads handles the cache miss while the other
    //    waits for the result to become available
    // 2. the waiting thread returns the result as soon as it is published
    //    by the other thread (before its cost is computed and cache cost
    //    update procedures are executed).
    zim::ConcurrentCache<int, size_t, CostAs3xValue> cache(1000);

    populateCache(cache, { {5, 50}, {10, 100}, {15, 150} } );

    const std::string targetOutput =
R"(thread#0: Output of interest starts from the next line
a  : ConcurrentCache::getOrPut(1) {
  b: ConcurrentCache::getOrPut(1) {
a  :  ConcurrentCache::getCacheSlot(1) {
a  :   entered synchronized section
  b:  ConcurrentCache::getCacheSlot(1) {
a  :   lru_cache::getOrPut(1) {
a  :    not in cache, adding...
a  :    lru_cache::putMissing(1) {
a  :     lru_cache::increaseCost(0) {
a  :      _current_cost after increase: 900
a  :      settled _current_cost: 900
a  :     }
a  :    }
a  :   }
a  :   exiting synchronized section
a  :  }
a  :  Obtained the cache slot
  b:   entered synchronized section
a  :  It was a cache miss. Going to obtain the value...
  b:   lru_cache::getOrPut(1) {
  b:    already in cache, moved to the beginning of the LRU list.
  b:   }
  b:   exiting synchronized section
  b:  }
  b:  Obtained the cache slot
  b:  Waiting for result...
a  :  Value was successfully obtained.
a  :  Made the value available for concurrent access.
  b: } (return value: 10)
a  :  Computing the cost of the new entry...
a  :  cost=30
a  :  ConcurrentCache::finalizeCacheMiss(1) {
a  :   entered synchronized section
a  :   lru_cache::put(1) {
a  :    lru_cache::decreaseCost(0) {
a  :     _current_cost after decrease: 900
a  :    }
a  :    lru_cache::increaseCost(30) {
a  :     _current_cost after increase: 930
a  :     settled _current_cost: 930
a  :    }
a  :   }
a  :   exiting synchronized section
a  :  }
a  :  Done. Cache cost is at 930
a  :  Returning immediately...
a  : } (return value: 10)
)";

    zim::Logging::logIntoMemory();
    zim::Logging::orchestrateConcurrentExecutionVia(targetOutput);

    const auto accessKey1 = [&cache]() { cache.getOrPut(1, LazyValue(10)); };
    log_debug("Output of interest starts from the next line");
    zim::NamedThread thread1("a  ", accessKey1 );
    zim::NamedThread thread2("  b", accessKey1 );
    thread1.join();
    thread2.join();

    ASSERT_EQ(zim::Logging::getInMemLogContent(), targetOutput);
}

TEST(ConcurrentCacheMultithreadedTest, concurrentTurmoil) {
    // This test simulates a flow in which handling of a cache miss takes long
    // enough for the newly allocated cache entry to be pushed all the way
    // through the LRU queue by concurrent cache hits so that by the time
    // the new item is materialized its slot has already been dropped from
    // the cache.
    const std::string targetOutput =
R"(thread#0: Output of interest starts from the next line
s  : ConcurrentCache::getOrPut(6) {
s  :  ConcurrentCache::getCacheSlot(6) {
s  :   entered synchronized section
s  :   lru_cache::getOrPut(6) {
s  :    not in cache, adding...
s  :    lru_cache::putMissing(6) {
s  :     lru_cache::increaseCost(0) {
s  :      _current_cost after increase: 150
s  :      settled _current_cost: 150
s  :     }
s  :    }
s  :   }
s  :   exiting synchronized section
s  :  }
s  :  Obtained the cache slot
s  :  It was a cache miss. Going to obtain the value...
  f: ConcurrentCache::getOrPut(5) {
  f:  ConcurrentCache::getCacheSlot(5) {
  f:   entered synchronized section
  f:   lru_cache::getOrPut(5) {
  f:    already in cache, moved to the beginning of the LRU list.
  f:   }
  f:   exiting synchronized section
  f:  }
  f:  Obtained the cache slot
  f:  Returning immediately...
  f: } (return value: 50)
  f: ConcurrentCache::getOrPut(2) {
  f:  ConcurrentCache::getCacheSlot(2) {
  f:   entered synchronized section
  f:   lru_cache::getOrPut(2) {
  f:    not in cache, adding...
  f:    lru_cache::putMissing(2) {
  f:     lru_cache::increaseCost(0) {
  f:      _current_cost after increase: 150
  f:      settled _current_cost: 150
  f:     }
  f:    }
  f:   }
  f:   exiting synchronized section
  f:  }
  f:  Obtained the cache slot
  f:  It was a cache miss. Going to obtain the value...
  f:  Value was successfully obtained.
  f:  Made the value available for concurrent access.
  f:  Computing the cost of the new entry...
  f:  cost=60
  f:  ConcurrentCache::finalizeCacheMiss(2) {
  f:   entered synchronized section
  f:   lru_cache::put(2) {
  f:    lru_cache::decreaseCost(0) {
  f:     _current_cost after decrease: 150
  f:    }
  f:    lru_cache::increaseCost(60) {
  f:     _current_cost after increase: 210
  f:     lru_cache::dropLast() {
  f:      evicting entry with key: 6
  f:      lru_cache::decreaseCost(0) {
  f:       _current_cost after decrease: 210
  f:      }
  f:     }
  f:     lru_cache::dropLast() {
  f:      evicting entry with key: 5
  f:      lru_cache::decreaseCost(150) {
  f:       _current_cost after decrease: 60
  f:      }
  f:     }
  f:     settled _current_cost: 60
  f:    }
  f:   }
  f:   exiting synchronized section
  f:  }
  f:  Done. Cache cost is at 60
  f:  Returning immediately...
  f: } (return value: 20)
 x : ConcurrentCache::getOrPut(6) {
 x :  ConcurrentCache::getCacheSlot(6) {
 x :   entered synchronized section
 x :   lru_cache::getOrPut(6) {
 x :    not in cache, adding...
 x :    lru_cache::putMissing(6) {
 x :     lru_cache::increaseCost(0) {
 x :      _current_cost after increase: 60
 x :      settled _current_cost: 60
 x :     }
 x :    }
 x :   }
 x :   exiting synchronized section
 x :  }
 x :  Obtained the cache slot
 x :  It was a cache miss. Going to obtain the value...
s  :  Value was successfully obtained.
s  :  Made the value available for concurrent access.
s  :  Computing the cost of the new entry...
s  :  cost=180
s  :  ConcurrentCache::finalizeCacheMiss(6) {
s  :   entered synchronized section
s  :   lru_cache::put(6) {
s  :    lru_cache::decreaseCost(0) {
s  :     _current_cost after decrease: 60
s  :    }
s  :    lru_cache::increaseCost(180) {
s  :     _current_cost after increase: 240
s  :     lru_cache::dropLast() {
s  :      evicting entry with key: 2
s  :      lru_cache::decreaseCost(60) {
s  :       _current_cost after decrease: 180
s  :      }
s  :     }
s  :     settled _current_cost: 180
s  :    }
s  :   }
s  :   exiting synchronized section
s  :  }
s  :  Done. Cache cost is at 180
s  :  Returning immediately...
s  : } (return value: 60)
 x :  Value was successfully obtained.
 x :  Made the value available for concurrent access.
 x :  Computing the cost of the new entry...
 x :  cost=180
 x :  ConcurrentCache::finalizeCacheMiss(6) {
 x :   entered synchronized section
 x :   lru_cache::put(6) {
 x :    lru_cache::decreaseCost(180) {
 x :     _current_cost after decrease: 0
 x :    }
 x :    lru_cache::increaseCost(180) {
 x :     _current_cost after increase: 180
 x :     settled _current_cost: 180
 x :    }
 x :   }
 x :   exiting synchronized section
 x :  }
 x :  Done. Cache cost is at 180
 x :  Returning immediately...
 x : } (return value: 60)
)";

    zim::ConcurrentCache<int, size_t, CostAs3xValue> cache(200);
    populateCache(cache, { {5, 50} } );

    zim::Logging::logIntoMemory();
    zim::Logging::orchestrateConcurrentExecutionVia(targetOutput);

    const auto slowCacheMiss = [&cache](){
      cache.getOrPut(6, LazyValue(60, std::chrono::milliseconds(10)));
    };

    const auto aBurstOfFastCacheAccesses = [&cache](){
      populateCache(cache, { {5, 50}, {2, 20} } );
    };

    log_debug("Output of interest starts from the next line");
    zim::NamedThread thread1("s  ", slowCacheMiss );
    zim::NamedThread thread2("  f", aBurstOfFastCacheAccesses );
    zim::NamedThread thread3(" x ", slowCacheMiss );
    thread1.join();
    thread2.join();
    thread3.join();

    ASSERT_EQ(zim::Logging::getInMemLogContent(), targetOutput);
}
