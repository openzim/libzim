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
  ASSERT_EQ(it.getSize(), -1);
  ASSERT_THROW(it.getZimId(), std::runtime_error);
  ASSERT_THROW(*it, std::runtime_error);
  ASSERT_THROW(it.operator->(), std::runtime_error);
}

TEST(search_iterator, end) {
  TempZimArchive tza("testZim");

  zim::Archive archive = tza.createZimFromTitles({
    "item a",
  });

  zim::Searcher searcher(archive);
  zim::Query query;
  query.setQuery("item", true);
  auto search = searcher.search(query);
  auto result = search.getResults(0, archive.getEntryCount());

  auto it = result.end();

  ASSERT_THROW(it.getTitle(), std::runtime_error);
  ASSERT_THROW(it.getPath(), std::runtime_error);
  ASSERT_EQ(it.getSnippet(), "");
//  ASSERT_EQ(it.getScore(), 0); Unspecified, may be 0 or 1. To fix.
  ASSERT_EQ(it.getFileIndex(), 0);
  ASSERT_EQ(it.getWordCount(), -1);
  ASSERT_EQ(it.getSize(), -1);
  ASSERT_THROW(*it, std::runtime_error);
  ASSERT_THROW(it.operator->(), std::runtime_error);
}

TEST(search_iterator, copy) {
  TempZimArchive tza("testZim");

  zim::Archive archive = tza.createZimFromTitles({
    "item a",
  });

  zim::Searcher searcher(archive);
  zim::Query query;
  query.setQuery("item", true);
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

  zim::Archive archive = tza.createZimFromTitles({
    "item a",
    "Item B",
    "iTem ć"
  });

  zim::Searcher searcher(archive);
  zim::Query query;
  query.setQuery("item", true);
  auto search = searcher.search(query);
  auto result = search.getResults(0, archive.getEntryCount());

  auto it = result.begin();

  // Test functions
  ASSERT_EQ(it.getTitle(), "item a");
  ASSERT_EQ(it.getPath(), "dummyPathitem a");
  ASSERT_EQ(it.getScore(), 100);
  ASSERT_EQ(it.getFileIndex(), 0);
  ASSERT_EQ(it.getZimId(), archive.getUuid());
  ASSERT_EQ(it.getWordCount(), -1);            // Unimplemented
  ASSERT_EQ(it.getSize(), -1);                 // Unimplemented

  // Check getTitle for accents/cased text
  it++;
  ASSERT_EQ(it.getTitle(), "Item B");
  it++;
  ASSERT_EQ(it.getTitle(), "iTem ć");
}

TEST(search_iterator, stemmedSearch) {
  TempZimArchive tza("testZim");

  // The following stemming occurs
  // apple -> appl
  // charlie -> charli
  // chocolate -> chocol
  // factory -> factori
  zim::Archive archive = tza.createZimFromTitles({
    "an apple a day, keeps the doctor away",
    "charlie and the chocolate factory"
  });

  zim::Searcher searcher(archive);
  zim::Query query;
  query.setQuery("apples", true);
  auto search = searcher.search(query);
  auto result = search.getResults(0, 1);

  ASSERT_EQ(result.begin().getSnippet(), "an <b>apple</b> a day, keeps the doctor away");

  query.setQuery("chocolate factory", true);
  search = searcher.search(query);
  result = search.getResults(0, 1);
  ASSERT_EQ(result.begin().getSnippet(), "charlie and the <b>chocolate</b> <b>factory</b>");
}

TEST(search_iterator, iteration) {
  TempZimArchive tza("testZim");

  zim::Archive archive = tza.createZimFromTitles({
    "item a",
    "item b"
  });

  zim::Searcher searcher(archive);
  zim::Query query;
  query.setQuery("item", true);
  auto search = searcher.search(query);
  auto result = search.getResults(0, archive.getEntryCount());

  auto it = result.begin();
  ASSERT_EQ(it.getTitle(), result.begin().getTitle());

  ASSERT_EQ(it.getTitle(), "item a");
  it++;
  ASSERT_EQ(it.getTitle(), "item b");
  ASSERT_TRUE(it != result.begin());

  it--;
  ASSERT_EQ(it.getTitle(), "item a");
  ASSERT_TRUE(result.begin() == it);

  it++; it++;
  ASSERT_TRUE(it == result.end());
}

} // anonymous namespace
