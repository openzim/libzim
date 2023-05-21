/*
 * Copyright (C) 2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#include "../src/tools.h"

#include "gtest/gtest.h"
#include <sstream>

namespace {
  TEST(Tools, wordCount) {
    ASSERT_EQ(zim::countWords(""), 0);
    ASSERT_EQ(zim::countWords("   "), 0);
    ASSERT_EQ(zim::countWords("One"), 1);
    ASSERT_EQ(zim::countWords("One Two Three"), 3);
    ASSERT_EQ(zim::countWords("  One  "), 1);
    ASSERT_EQ(zim::countWords("One    Two Three   "), 3);
    ASSERT_EQ(zim::countWords("One.Two\tThree"), 2);
  }


  TEST(Tools, parseIllustrationPathToSize) {
    ASSERT_EQ(zim::parseIllustrationPathToSize("Illustration_0x0@1"), 0);
    ASSERT_EQ(zim::parseIllustrationPathToSize("Illustration_1x1@1"), 1);
    ASSERT_EQ(zim::parseIllustrationPathToSize("Illustration_01x01@1"), 1);
    ASSERT_EQ(zim::parseIllustrationPathToSize("Illustration_64x64@1"), 64);
    ASSERT_EQ(zim::parseIllustrationPathToSize("Illustration_128x128@1"), 128);
    ASSERT_EQ(zim::parseIllustrationPathToSize("Illustration_1024x1024@1"), 1024);
    ASSERT_THROW(zim::parseIllustrationPathToSize("Illsration_64x64@1"), std::runtime_error);
    ASSERT_THROW(zim::parseIllustrationPathToSize("Illstration_"), std::runtime_error);
    ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_64x@1"), std::runtime_error);
    ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_64x"), std::runtime_error);
    ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_64x64"), std::runtime_error);
    ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_64x64@1.5"), std::runtime_error);
    ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_128x64@1"), std::runtime_error);
    ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_-32x-32@1"), std::runtime_error);
    ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_ 64x64@1"), std::runtime_error);
    ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_64x 64@1"), std::runtime_error);
    ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_ 64x 64@1"), std::runtime_error);
    ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_1 28x1 28@1"), std::runtime_error);
  }

#if defined(ENABLE_XAPIAN)
  TEST(Tools, removeAccents) {
    ASSERT_EQ(zim::removeAccents("bépoàǹ"), "bepoan");
    std::ostringstream ss;
    for(auto i=0; i<4*1024; i++) {
      ss << "bépo";
    }
    auto accentedString(ss.str());
    ss.str("");
    for(auto i=0; i<4*1024; i++) {
      ss << "bepo";
    }
    auto unaccentedString(ss.str());
    ASSERT_EQ(zim::removeAccents(accentedString), unaccentedString);
  }
#endif
}
