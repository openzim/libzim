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
#include <string>

#if defined(ENABLE_XAPIAN)
  #include <unicode/unistr.h>
#endif

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
    // Create 2 and half batches (3 boundaries) of 4kb.
    // Each bondary has its property:
    // - A 4 bytes chars being cut by the boundary
    // - A "é" stored in NDF form when the "e" is before the boundary and the "´" is after
    // - Nothing special
    for(auto j=0; j<1023; j++) {
      ss << "bépo";
    }
    ss << "bép𩸽";
    for(auto j=0; j<1023; j++) {
      ss << "bépo";
    }
    ss << "bép" << "e" << "\xcc\x81";
    for (auto j=0; j<512; j++) {
      ss << "bépo";
    }
    auto accentedString(ss.str());
    // Check our input data (that we have a char in the middle of a batch boundary)
    // Indexing is made on u16
    icu::UnicodeString ustring(accentedString.c_str());

    // Test input data.
    // "bépo" is 4 chars
    ASSERT_EQ(ustring.getChar32Limit(0), 0);
    ASSERT_EQ(ustring.getChar32Limit(1), 1);
    ASSERT_EQ(ustring.getChar32Limit(2), 2);
    ASSERT_EQ(ustring.getChar32Limit(3), 3);
    ASSERT_EQ(ustring.getChar32Limit(4), 4);
    // 𩸽 is in the middle of a boundary
    ASSERT_EQ(ustring.getChar32Limit(4*1024-1), 4*1024-1);
    ASSERT_EQ(ustring.getChar32Limit(4*1024), 4*1024+1);
    ASSERT_EQ(ustring.getChar32Limit(4*1024+1), 4*1024+1);
    // Because of 𩸽 at first boundary, second boundary will search at (4*1024+1) + 4*1024 so 8*1024+1
    ASSERT_EQ(ustring.getChar32Limit(8*1024), 8*1024);
    ASSERT_EQ(ustring.getChar32Limit(8*1024+1), 8*1024+1);
    ASSERT_EQ(ustring.getChar32Limit(8*1024+2), 8*1024+2);
    // boundary is in the middle of "e´"
    EXPECT_EQ(ustring[8*1024-1], 'p'); // boundary - 2
    EXPECT_EQ(ustring[8*1024], 'e'); // boundary - 1
    EXPECT_EQ(ustring[8*1024+1], 0x0301); // boundary. Unicode symbol for '´' (utf16: 0x0301, utf8 : 0xCC 0x81)
    EXPECT_EQ(ustring[8*1024+2], 'b'); // boundary + 1
    EXPECT_EQ(ustring[8*1024+3], 233 /*ascii code for é*/); // boundary + 2
    ss.str("");
    for(auto j=0; j<1023; j++) {
      ss << "bepo";
    }
    ss << "bep𩸽";
    for(auto j=0; j<1023; j++) {
      ss << "bepo";
    }
    ss << "bep" << "e";
    for (auto j=0; j<512; j++) {
      ss << "bepo";
    }
    auto unaccentedString(ss.str());
    ASSERT_EQ(zim::removeAccents(accentedString), unaccentedString);
  }

  TEST(Tools, simple_remove_accents) {
    ASSERT_EQ(zim::removeAccents("jàzz"), "jazz");
  }
#endif
}


