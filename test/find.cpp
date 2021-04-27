/*
 * Copyright (C) 2009 Miguel Rocha
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

#include <zim/zim.h>
#include <zim/archive.h>
#include <zim/error.h>

#include "gtest/gtest.h"

namespace
{
// Not found cases

// ByTitle
#if WITH_TEST_DATA
TEST(FindTests, NotFoundByTitle)
{
    zim::Archive archive ("./data/wikibooks_be_all_nopic_2017-02.zim");

    auto range0 = archive.findByTitle("unkownTitle");
    auto range1 = archive.findByTitle("j/body.js");
    ASSERT_EQ(range0.begin(), range0.end());
    ASSERT_EQ(range1.begin(), range1.end());
}

// By Path
TEST(FindTests, NotFoundByPath)
{
    zim::Archive archive ("./data/wikibooks_be_all_nopic_2017-02.zim");

    auto range0 = archive.findByPath("unkwonUrl");
    auto range1 = archive.findByPath("U/unkwonUrl");
    auto range2 = archive.findByPath("A/unkwonUrl");
    auto range3 = archive.findByPath("X");
    auto range4 = archive.findByPath("X/");
    ASSERT_EQ(range0.begin(), range0.end());
    ASSERT_EQ(range1.begin(), range1.end());
    ASSERT_EQ(range2.begin(), range2.end());
    ASSERT_EQ(range3.begin(), range3.end());
    ASSERT_EQ(range4.begin(), range4.end());
}

// Found cases

// ByTitle
TEST(FindTests, ByTitle)
{
    zim::Archive archive ("./data/wikibooks_be_all_nopic_2017-02.zim");

    auto range0 = archive.findByTitle("Main Page");
    ASSERT_EQ(range0.begin()->getIndex(), 5);
    ASSERT_EQ(range0.end()->getIndex(), 7); // getIndex of an entry is always the path order.
                                            // It happens that the 6th in title order is the 7th in path order.

    auto count = 0;
    for(auto& entry: range0) {
      count++;
      ASSERT_EQ(entry.getTitle().find("Main Page"), 0);
    }
    ASSERT_EQ(count, 1);

    auto range1 = archive.findByTitle("Украінская");
    ASSERT_EQ(range1.begin()->getIndex(), 53);
    ASSERT_EQ(range1.end()->getIndex(), 58);

    count = 0;
    for(auto& entry: range1) {
      count++;
      ASSERT_EQ(entry.getTitle().find("Украінская"), 0);
    }
    ASSERT_EQ(count, 5);
}

#if 0
// By Path (compatibility)
TEST(FindTests, ByPathNoNS)
{
    zim::Archive archive ("./data/wikibooks_be_all_nopic_2017-02.zim");

    auto it1 = archive.findByPath("j/body.js");
    auto it2 = archive.findByPath("m/115a35549794e50dcd03e60ef1a1ae24.png");
    ASSERT_EQ(it1->getIndex(), 1);
    ASSERT_EQ(it2->getIndex(), 76);
}
#endif

// By Path
TEST(FindTests, ByPath)
{
    zim::Archive archive ("./data/wikibooks_be_all_nopic_2017-02.zim");

    auto range0 = archive.findByPath("A/Main_Page.html");
    auto range1 = archive.findByPath("I/s/");
    auto range2 = archive.findByPath("-/j/head.js");
    auto range3 = archive.findByPath("I");
    auto range4 = archive.findByPath("I/");
    auto range5 = archive.findByPath("");
    auto range6 = archive.findByPath("/");

    ASSERT_EQ(range0.begin()->getIndex(), 5);
    auto count = 0;
    for(auto& entry: range0) {
      count++;
      ASSERT_EQ(entry.getPath().find("A/Main_Page.html"), 0);
    }
    ASSERT_EQ(count, 1);

    ASSERT_EQ(range1.begin()->getIndex(), 78);
    count = 0;
    for(auto& entry: range1) {
      count++;
      std::cout << entry.getPath() << std::endl;
      ASSERT_EQ(entry.getPath().find("I/s/"), 0);
    }
    ASSERT_EQ(count, 31);

    ASSERT_EQ(range2.begin()->getIndex(), 2);
    count = 0;
    for(auto& entry: range2) {
      count++;
      ASSERT_EQ(entry.getPath().find("-/j/head.js"), 0);
    }
    ASSERT_EQ(count, 1);

    ASSERT_EQ(range3.begin()->getIndex(), 75);
    count = 0;
    for(auto& entry: range3) {
      count++;
      std::cout << entry.getPath() << std::endl;
      ASSERT_EQ(entry.getPath().find("I"), 0);
    }
    ASSERT_EQ(count, 34);

    ASSERT_EQ(range4.begin()->getIndex(), 75);
    count = 0;
    for(auto& entry: range4) {
      count++;
      std::cout << entry.getPath() << std::endl;
      ASSERT_EQ(entry.getPath().find("I/"), 0);
    }
    ASSERT_EQ(count, 34);

    count = 0;
    for(auto& entry: range5) {
      ASSERT_EQ(count, entry.getIndex());
      count++;
    }
    ASSERT_EQ(count, 118);

    count = 0;
    for(auto& entry: range6) {
      ASSERT_EQ(count, entry.getIndex());
      count++;
    }
    ASSERT_EQ(count, 118);
}
#endif

} // namespace
