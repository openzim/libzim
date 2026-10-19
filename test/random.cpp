/*
 * Copyright (C) 2021 Matthieu Gautier mgautier@kymeria.fr
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

#include "gtest/gtest.h"
#include <cstdint>

namespace zim {
  uint32_t randomNumber(uint32_t max);
};

using namespace zim;

namespace
{
TEST(Random, smallMax)
{
  for(auto i=0; i<1000; i++) {
    ASSERT_EQ(randomNumber(0), 0U);
  }


  constexpr uint32_t NB_TOSSES = 1000;
  constexpr uint32_t MAX_IMBALANCE = 154;
  uint32_t zeroes = 0;
  uint32_t ones = 0;
  for (auto i = 0U; i < NB_TOSSES; i++) {
    auto r = randomNumber(1);
    ASSERT_LE(r, 1U) << r;
    if (r == 0U) {
      zeroes++;
    } else {
      ones++;
    }
  }
  // If X ~ Binomial(n, 1/2), the imbalance D = |#0 - #1| = |2X - n|.
  // For n = 1000, P(D > 154) = 2 * sum(k=0..422) C(1000, k) / 2^1000
  // = 9.06e-7, so this limit keeps the test failure probability below 1e-6.
  const auto imbalance = zeroes > ones ? zeroes - ones : ones - zeroes;
  ASSERT_LE(imbalance, MAX_IMBALANCE);
}

TEST(Random, distribution)
{
  const uint32_t NB_NUMBERS = 1000000;
  const uint32_t NB_BUCKETS = 100;
  const uint32_t BUCKET_SIZE = NB_NUMBERS/NB_BUCKETS;
  const uint32_t MAX_RANDOM = 1000000;
  std::vector<uint32_t> distribution(NB_BUCKETS);

  for (auto i=0U; i<NB_NUMBERS; i++) {
    auto r = randomNumber(MAX_RANDOM);
    auto bucket_index = (int)((float)r / MAX_RANDOM * NB_BUCKETS);
    if (bucket_index == NB_BUCKETS) {
      // This only happens when r == MAX_RANDOM.
      bucket_index = NB_BUCKETS-1;
    }
    distribution[bucket_index]++;
  }
  // Each bucket should have around BUCKET_SIZE element.
  // Test this is true at 10%
  for(auto nbElement:distribution) {
    ASSERT_GT(nbElement, BUCKET_SIZE*0.9);
    ASSERT_LT(nbElement, BUCKET_SIZE*1.1);
  }
}


};
