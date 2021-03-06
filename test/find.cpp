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

#include "tools.h"

#include "gtest/gtest.h"

namespace
{
// Not found cases


using zim::unittests::getDataFilePath;

// ByTitle
#if WITH_TEST_DATA
TEST(FindTests, NotFoundByTitle)
{
    for(auto& testfile:getDataFilePath("wikibooks_be_all_nopic_2017-02.zim")) {
        zim::Archive archive (testfile.path);

        auto range0 = archive.findByTitle("unkownTitle");
        auto range1 = archive.findByTitle("j/body.js");
        ASSERT_EQ(range0.begin(), range0.end());
        ASSERT_EQ(range1.begin(), range1.end());
    }
}

// By Path
TEST(FindTests, NotFoundByPath)
{
    for(auto& testfile:getDataFilePath("wikibooks_be_all_nopic_2017-02.zim")) {
        zim::Archive archive (testfile.path);

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
}

// Found cases

// ByTitle
TEST(FindTests, ByTitle)
{
    for(auto& testfile:getDataFilePath("wikibooks_be_all_nopic_2017-02.zim")) {
        zim::Archive archive (testfile.path);

        auto range0 = archive.findByTitle("Першая старонка");

        auto count = 0;
        for(auto& entry: range0) {
          count++;
          ASSERT_EQ(entry.getTitle().find("Першая старонка"), 0);
        }
        ASSERT_EQ(count, 1);

        auto range1 = archive.findByTitle("Украінская");

        count = 0;
        for(auto& entry: range1) {
          count++;
          ASSERT_EQ(entry.getTitle().find("Украінская"), 0);
        }
        ASSERT_EQ(count, 5);
    }
}

// By Path
TEST(FindTests, ByPath)
{
  for(auto& testfile:getDataFilePath("wikibooks_be_all_nopic_2017-02.zim", "withns")) {
    zim::Archive archive (testfile.path);

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
}

// By Path
TEST(FindTests, ByPathNons)
{
  for(auto& testfile:getDataFilePath("wikibooks_be_all_nopic_2017-02.zim", "nons")) {
    zim::Archive archive (testfile.path);

    auto range0 = archive.findByPath("Першая_старонка.html");
    auto range1 = archive.findByPath("П");
    auto range2 = archive.findByPath("");
    auto range3 = archive.findByPath("/");

    auto count = 0;
    for(auto& entry: range0) {
      count++;
      ASSERT_EQ(entry.getPath().find("Першая_старонка.html"), 0);
    }
    ASSERT_EQ(count, 1);

    count = 0;
    for(auto& entry: range1) {
      count++;
      std::cout << entry.getPath() << std::endl;
      ASSERT_EQ(entry.getPath().find("П"), 0);
    }
    ASSERT_EQ(count, 2);

    count = 0;
    for(auto& entry: range2) {
      ASSERT_EQ(count, entry.getIndex());
      count++;
    }
    ASSERT_EQ(count, 109);

    count = 0;
    for(auto& entry: range3) {
      ASSERT_EQ(count, entry.getIndex());
      count++;
    }
    ASSERT_EQ(count, 109);
  }
}
#endif

} // namespace
