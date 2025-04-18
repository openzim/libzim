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
