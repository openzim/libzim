/*
 * Copyright (C) 2021 Maneesh P M <manu.pm55@gmail.com>
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

#define ZIM_PRIVATE
#include <zim/archive.h>
#include <zim/search.h>
#include <zim/search_iterator.h>
#include <zim/error.h>
#include "tools.h"

#include "gtest/gtest.h"

namespace {

using zim::unittests::TempZimArchive;

TEST(search_iterator, uninitialized) {
  zim::SearchResultSet::iterator it;
  ASSERT_EQ(it.getTitle(), "");
  ASSERT_EQ(it.getPath(), "");
  ASSERT_EQ(it.getSnippet(), "");
  ASSERT_EQ(it.getScore(), 0);
  ASSERT_EQ(it.getFileIndex(), 0);
  ASSERT_EQ(it.getWordCount(), -1);
  ASSERT_THROW(it.getZimId(), std::runtime_error);
  ASSERT_THROW(*it, std::runtime_error);
  ASSERT_THROW(it.operator->(), std::runtime_error);
}

#if defined(ENABLE_XAPIAN_CREATOR)
TEST(search_iterator, end) {
  TempZimArchive tza("testZim");

  zim::Archive archive = tza.createZimFromContent({
    {"article 1", "item a"}
  });

  zim::Searcher searcher(archive);
  zim::Query query("item");
  auto search = searcher.search(query);
  auto result = search.getResults(0, archive.getEntryCount());

  auto it = result.end();

  ASSERT_THROW(it.getTitle(), std::runtime_error);
  ASSERT_THROW(it.getPath(), std::runtime_error);
  ASSERT_THROW(it.getSnippet(), std::runtime_error);
  ASSERT_THROW(it.getScore(), std::runtime_error);
  ASSERT_THROW(it.getFileIndex(), std::runtime_error);
  ASSERT_THROW(it.getWordCount(), std::runtime_error);
  ASSERT_THROW(*it, std::runtime_error);
  ASSERT_THROW(it.operator->(), std::runtime_error);
}

TEST(search_iterator, copy) {
  TempZimArchive tza("testZim");

  zim::Archive archive = tza.createZimFromContent({
    {"article 1", "item a"}
  });

  zim::Searcher searcher(archive);
  zim::Query query(std::string("item"));
  auto search = searcher.search(query);
  auto result = search.getResults(0, archive.getEntryCount());

  auto it = result.begin();

  auto it2 = it;
  ASSERT_EQ(it.getTitle(), it2.getTitle());

  it = result.end();
  it2 = it;
  ASSERT_EQ(it, it2);
  ASSERT_THROW(it.getTitle(), std::runtime_error);
  ASSERT_THROW(it2.getTitle(), std::runtime_error);
}

TEST(search_iterator, functions) {
  TempZimArchive tza("testZim");

  zim::Archive archive = tza.createZimFromContent({
    {"item a", "item item item"},
    {"Item B", "item item 2"},
    {"iTem ć", "item number 3"}  // forcing an order using wdf
  });

  zim::Searcher searcher(archive);
  zim::Query query("item");
  auto search = searcher.search(query);
  auto result = search.getResults(0, archive.getEntryCount());

  auto it = result.begin();

  // Test functions
  ASSERT_EQ(it.getTitle(), "item a");
  ASSERT_EQ(it.getPath(), "dummyPathitem a");
  ASSERT_EQ(it.getScore(), 100);
  ASSERT_EQ(it.getFileIndex(), 0);
  ASSERT_EQ(it.getZimId(), archive.getUuid());
  ASSERT_EQ(it.getWordCount(), 3);

  // Check getTitle for accents/cased text
  it++;
  ASSERT_EQ(it.getTitle(), "Item B");
  it++;
  ASSERT_EQ(it.getTitle(), "iTem ć");
}

TEST(search_iterator, iteration) {
  TempZimArchive tza("testZim");

  zim::Archive archive = tza.createZimFromContent({
    {"article 1", "item"},
    {"article 2", "another item in article 2"}  // different wdf
  });

  zim::Searcher searcher(archive);
  auto search = searcher.search(std::string("item"));
  auto result = search.getResults(0, archive.getEntryCount());

  auto it = result.begin();
  ASSERT_EQ(it.getTitle(), result.begin().getTitle());

  ASSERT_EQ(it.getTitle(), "article 1");
  it++;
  ASSERT_EQ(it.getTitle(), "article 2");
  ASSERT_TRUE(it != result.begin());

  it--;
  ASSERT_EQ(it.getTitle(), "article 1");
  ASSERT_TRUE(result.begin() == it);

  it++; it++;
  ASSERT_TRUE(it == result.end());
}
#endif

} // anonymous namespace
