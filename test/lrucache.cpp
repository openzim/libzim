/*
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
 */

#include "lrucache.h"
#include "concurrent_cache.h"
#include "gtest/gtest.h"

const int NUM_OF_TEST2_RECORDS = 100;
const unsigned int TEST2_CACHE_CAPACITY = 50u;
const unsigned int TEST2_CACHE_CAPACITY_SMALL = 10u;

TEST(CacheTest, SimplePut) {
    zim::lru_cache<int, int> cache_lru(1);
    cache_lru.put(7, 777);
    EXPECT_TRUE(cache_lru.exists(7));
    EXPECT_EQ(777, cache_lru.get(7));
    EXPECT_EQ(1u, cache_lru.size());
}

TEST(CacheTest, OverwritingPut) {
    zim::lru_cache<int, int> cache_lru(1);
    cache_lru.put(7, 777);
    cache_lru.put(7, 222);
    EXPECT_TRUE(cache_lru.exists(7));
    EXPECT_EQ(222, cache_lru.get(7));
    EXPECT_EQ(1u, cache_lru.size());
}

TEST(CacheTest, MissingValue) {
    zim::lru_cache<int, int> cache_lru(1);
    EXPECT_TRUE(cache_lru.get(7).miss());
    EXPECT_FALSE(cache_lru.get(7).hit());
    EXPECT_THROW(cache_lru.get(7).value(), std::range_error);
}

TEST(CacheTest, DropValue) {
    zim::lru_cache<int, int> cache_lru(3);
    cache_lru.put(7, 777);
    cache_lru.put(8, 888);
    cache_lru.put(9, 999);
    EXPECT_EQ(3u, cache_lru.size());
    EXPECT_TRUE(cache_lru.exists(7));
    EXPECT_EQ(777, cache_lru.get(7));

    EXPECT_TRUE(cache_lru.drop(7));

    EXPECT_EQ(2u, cache_lru.size());
    EXPECT_FALSE(cache_lru.exists(7));
    EXPECT_THROW(cache_lru.get(7).value(), std::range_error);

    EXPECT_FALSE(cache_lru.drop(7));
}

#define EXPECT_RANGE_MISSING_FROM_CACHE(CACHE, START, END) \
for (unsigned i = START; i  < END; ++i)  {                 \
  EXPECT_FALSE(CACHE.exists(i));                           \
}

#define EXPECT_RANGE_FULLY_IN_CACHE(CACHE, START, END, VALUE_KEY_RATIO) \
for (unsigned i = START; i  < END; ++i)  {                              \
  EXPECT_TRUE(CACHE.exists(i));                                         \
  EXPECT_EQ(i*VALUE_KEY_RATIO, cache_lru.get(i));                       \
}



TEST(CacheTest1, KeepsAllValuesWithinCapacity) {
    zim::lru_cache<int, int> cache_lru(TEST2_CACHE_CAPACITY);

    for (int i = 0; i < NUM_OF_TEST2_RECORDS; ++i) {
        cache_lru.put(i, i);
    }

    EXPECT_RANGE_MISSING_FROM_CACHE(cache_lru, 0, (NUM_OF_TEST2_RECORDS - TEST2_CACHE_CAPACITY))

    EXPECT_RANGE_FULLY_IN_CACHE(cache_lru, (NUM_OF_TEST2_RECORDS - TEST2_CACHE_CAPACITY), NUM_OF_TEST2_RECORDS, 1)

    size_t size = cache_lru.size();
    EXPECT_EQ(TEST2_CACHE_CAPACITY, size);
}

TEST(CacheTest1, ChangeCacheCapacity) {
    zim::lru_cache<int, int> cache_lru(TEST2_CACHE_CAPACITY);

    for (int i = 0; i < NUM_OF_TEST2_RECORDS; ++i) {
        cache_lru.put(i, i);
    }

    EXPECT_EQ(TEST2_CACHE_CAPACITY, cache_lru.size());
    EXPECT_RANGE_MISSING_FROM_CACHE(cache_lru, 0, (NUM_OF_TEST2_RECORDS - TEST2_CACHE_CAPACITY))
    EXPECT_RANGE_FULLY_IN_CACHE(cache_lru, (NUM_OF_TEST2_RECORDS - TEST2_CACHE_CAPACITY), NUM_OF_TEST2_RECORDS, 1)

    cache_lru.setMaxSize(TEST2_CACHE_CAPACITY_SMALL);
    EXPECT_EQ(TEST2_CACHE_CAPACITY_SMALL, cache_lru.size());
    EXPECT_RANGE_MISSING_FROM_CACHE(cache_lru, 0, (NUM_OF_TEST2_RECORDS - TEST2_CACHE_CAPACITY_SMALL))
    EXPECT_RANGE_FULLY_IN_CACHE(cache_lru, (NUM_OF_TEST2_RECORDS - TEST2_CACHE_CAPACITY_SMALL), NUM_OF_TEST2_RECORDS, 1)

    cache_lru.setMaxSize(TEST2_CACHE_CAPACITY);
    for (int i = 0; i < NUM_OF_TEST2_RECORDS; ++i) {
        cache_lru.put(i, 1000*i);
    }
    EXPECT_EQ(TEST2_CACHE_CAPACITY, cache_lru.size());
    EXPECT_RANGE_MISSING_FROM_CACHE(cache_lru, 0, (NUM_OF_TEST2_RECORDS - TEST2_CACHE_CAPACITY))
    EXPECT_RANGE_FULLY_IN_CACHE(cache_lru, (NUM_OF_TEST2_RECORDS - TEST2_CACHE_CAPACITY), NUM_OF_TEST2_RECORDS, 1000)
}

TEST(ConcurrentCacheTest, handleException) {
    zim::ConcurrentCache<int, int> cache(1);
    auto val = cache.getOrPut(7, []() { return 777; });
    EXPECT_EQ(val, 777);
    EXPECT_THROW(cache.getOrPut(8, []() { throw std::runtime_error("oups"); return 0; }), std::runtime_error);
    val = cache.getOrPut(8, []() { return 888; });
    EXPECT_EQ(val, 888);
}
