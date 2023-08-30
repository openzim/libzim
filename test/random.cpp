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
    ASSERT_EQ(randomNumber(0), 0);
  }


  for(auto i=0; i<1000; i++) {
    auto r = randomNumber(1);
    ASSERT_TRUE(r>=0 && r<=1) << r;
  }
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
    auto bucket_index = (float)r / MAX_RANDOM * NB_BUCKETS;
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
