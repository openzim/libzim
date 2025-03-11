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
thread#0:  Value was successfully obtained. Computing its cost...
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
thread#0:  Value was successfully obtained. Computing its cost...
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
thread#0:  Value was successfully obtained. Computing its cost...
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
thread#0:  Value was successfully obtained. Computing its cost...
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
thread#0:  Value was successfully obtained. Computing its cost...
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

TEST(ConcurrentCacheMultithreadedTest, accessSameExistingItem) {
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
a  : } (return value: 50)
  b:    already in cache, moved to the beginning of the LRU list.
  b:   }
  b:   exiting synchronized section
  b:  }
  b:  Obtained the cache slot
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
