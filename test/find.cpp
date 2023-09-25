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
using zim::unittests::TempZimArchive;
using zim::unittests::TestItem;

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
          ASSERT_EQ(entry.getTitle().find("Першая старонка"), 0U);
        }
        if (testfile.category == "withns") {
          // On the withns test file, there are two entry with this title:
          //  the entry itself and the index.html (a redirection)
          ASSERT_EQ(count, 2);
        } else {
          // On new test file, the main page redirection is store in `W` namespace,
          // so the findByTitle found only 1 entry in `C` namespace.
          ASSERT_EQ(count, 1);
        }

        auto range1 = archive.findByTitle("Украінская");

        count = 0;
        for(auto& entry: range1) {
          count++;
          ASSERT_EQ(entry.getTitle().find("Украінская"), 0U);
        }
        ASSERT_EQ(count, 5);

        // Offset from end
        auto range2 = archive.findByTitle("Украінская");
        range2 = range2.offset(0, 2);
        count = 0;
        for(auto& entry: range2) {
          count++;
          ASSERT_EQ(entry.getTitle().find("Украінская"), 0U);
        }
        ASSERT_EQ(count, 2);

        // Offset from start
        auto range3 = archive.findByTitle("Украінская");
        range3 = range3.offset(1, 4);
        count = 0;
        for(auto& entry: range3) {
          count++;
          ASSERT_EQ(entry.getTitle().find("Украінская"), 0U);
        }
        ASSERT_EQ(count, 4);

        // Offset with more max results greater than the number of results
        auto range4 = archive.findByTitle("Украінская");
        range4 = range4.offset(0, 10);
        count = 0;
        for(auto& entry: range4) {
          count++;
          ASSERT_EQ(entry.getTitle().find("Украінская"), 0U);
        }
        ASSERT_EQ(count, 5);

        // Offset with start greater than the number of results
        auto range5 = archive.findByTitle("Украінская");
        range5 = range5.offset(10, 5);
        count = 0;
        for(auto& entry: range5) {
          count++;
          ASSERT_EQ(entry.getTitle().find("Украінская"), 0U);
        }
        ASSERT_EQ(count, 0);
    }
}

#define CHECK_FIND_TITLE_COUNT(prefix, expected_count) \
{ \
  auto count = 0; \
  auto range = archive.findByTitle(prefix); \
  for(auto& entry: range) { \
    count++; \
    ASSERT_EQ(entry.getTitle().find(prefix), 0U); \
  } \
  ASSERT_EQ(count, expected_count); \
}

TEST(FindTests, ByTitleWithDuplicate)
{
  TempZimArchive tza("testZim");
  zim::writer::Creator creator;
  creator.startZimCreation(tza.getPath());
  creator.addItem(std::make_shared<TestItem>("article0", "text/html", "AAA", ""));
  creator.addItem(std::make_shared<TestItem>("article1", "text/html", "BB", ""));
  creator.addItem(std::make_shared<TestItem>("article2", "text/html", "BBB", ""));
  creator.addItem(std::make_shared<TestItem>("article3", "text/html", "BBB", ""));
  creator.addItem(std::make_shared<TestItem>("article4", "text/html", "BBBB", ""));
  creator.addItem(std::make_shared<TestItem>("article5", "text/html", "CCC", ""));
  creator.addItem(std::make_shared<TestItem>("article6", "text/html", "CCC", ""));
  creator.finishZimCreation();

  zim::Archive archive(tza.getPath());
  // First binary seach step will look for index 3 (0+6/2) which is a BBB,
  // but we want to be sure it returns article2 which is the start of the range "BBB*"
  CHECK_FIND_TITLE_COUNT("BBB", 3)
  CHECK_FIND_TITLE_COUNT("BB", 4)
  CHECK_FIND_TITLE_COUNT("BBBB", 1)
  CHECK_FIND_TITLE_COUNT("CCC", 2)
  CHECK_FIND_TITLE_COUNT("C", 2)
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

    ASSERT_EQ(range0.begin()->getIndex(), 5U);
    unsigned count = 0;
    for(auto& entry: range0) {
      count++;
      ASSERT_EQ(entry.getPath().find("A/Main_Page.html"), 0U);
    }
    ASSERT_EQ(count, 1U);

    ASSERT_EQ(range1.begin()->getIndex(), 78U);
    count = 0;
    for(auto& entry: range1) {
      count++;
      std::cout << entry.getPath() << std::endl;
      ASSERT_EQ(entry.getPath().find("I/s/"), 0U);
    }
    ASSERT_EQ(count, 31U);

    ASSERT_EQ(range2.begin()->getIndex(), 2U);
    count = 0;
    for(auto& entry: range2) {
      count++;
      ASSERT_EQ(entry.getPath().find("-/j/head.js"), 0U);
    }
    ASSERT_EQ(count, 1U);

    ASSERT_EQ(range3.begin()->getIndex(), 75U);
    count = 0;
    for(auto& entry: range3) {
      count++;
      std::cout << entry.getPath() << std::endl;
      ASSERT_EQ(entry.getPath().find("I"), 0U);
    }
    ASSERT_EQ(count, 34U);

    ASSERT_EQ(range4.begin()->getIndex(), 75U);
    count = 0;
    for(auto& entry: range4) {
      count++;
      std::cout << entry.getPath() << std::endl;
      ASSERT_EQ(entry.getPath().find("I/"), 0U);
    }
    ASSERT_EQ(count, 34U);

    count = 0;
    for(auto& entry: range5) {
      ASSERT_EQ(count, entry.getIndex());
      count++;
    }
    ASSERT_EQ(count, 118U);

    count = 0;
    for(auto& entry: range6) {
      ASSERT_EQ(count, entry.getIndex());
      count++;
    }
    ASSERT_EQ(count, 118U);
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

    unsigned count = 0;
    for(auto& entry: range0) {
      count++;
      ASSERT_EQ(entry.getPath().find("Першая_старонка.html"), 0U);
    }
    ASSERT_EQ(count, 1U);

    count = 0;
    for(auto& entry: range1) {
      count++;
      std::cout << entry.getPath() << std::endl;
      ASSERT_EQ(entry.getPath().find("П"), 0U);
    }
    ASSERT_EQ(count, 2U);

    count = 0;
    for(auto& entry: range2) {
      ASSERT_EQ(count, entry.getIndex());
      count++;
    }
    ASSERT_EQ(count, 109U);

    count = 0;
    for(auto& entry: range3) {
      ASSERT_EQ(count, entry.getIndex());
      count++;
    }
    ASSERT_EQ(count, 109U);
  }
}
#endif

} // namespace
