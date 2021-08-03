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
#include <zim/suggestion.h>
#include <zim/suggestion_iterator.h>
#include <zim/error.h>
#include "tools.h"

#include "gtest/gtest.h"

namespace {

using zim::unittests::TempZimArchive;

TEST(suggestion_iterator, end) {
  TempZimArchive tza("testZim");

  zim::Archive archive = tza.createZimFromContent({
    {"article 1", "item a"}
  });

  zim::SuggestionSearcher searcher(archive);
  auto search = searcher.suggest("item");
  auto result = search.getResults(0, archive.getEntryCount());

  auto it = result.end();

  ASSERT_THROW(it.getEntry(), std::runtime_error);
  ASSERT_THROW(*it, std::runtime_error);
}

TEST(suggestion_iterator, copy) {
  TempZimArchive tza("testZim");

  zim::Archive archive = tza.createZimFromContent({
    {"article 1", "item a"}
  });

  zim::Searcher searcher(archive);
  zim::Query query;
  query.setQuery("item");
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

TEST(suggestion_iterator, functions) {
  TempZimArchive tza("testZim");

  zim::Archive archive = tza.createZimFromContent({
    {"article 1", "item a"}
  });

  zim::SuggestionSearcher searcher(archive);
  auto search = searcher.suggest("article");
  auto result = search.getResults(0, archive.getEntryCount());

  auto it = result.begin();

  // Test functions
  ASSERT_EQ(it->getTitle(), "article 1");
  ASSERT_EQ(it->getPath(), "dummyPatharticle 1");

  auto entry = it.getEntry();
  ASSERT_EQ(entry.getTitle(), "article 1");
}

TEST(suggestion_iterator, iteration) {
  TempZimArchive tza("testZim");

  zim::Archive archive = tza.createZimFromContent({
    {"article a", "item a"},
    {"article b", "item b"}
  });

  zim::SuggestionSearcher searcher(archive);
  auto search = searcher.suggest("article");
  auto result = search.getResults(0, archive.getEntryCount());
  auto it1 = result.begin();

  zim::SuggestionIterator it = it1;
  ASSERT_EQ(it->getTitle(), result.begin()->getTitle());

  ASSERT_EQ(it->getTitle(), "article a");
  it++;
  ASSERT_EQ(it->getTitle(), "article b");
  ASSERT_TRUE(it != it1);
  ASSERT_FALSE(it == it1);

  it--;
  ASSERT_EQ(it->getTitle(), "article a");
  ASSERT_TRUE(result.begin() == it);
  it++; it++;
  ASSERT_TRUE(it == result.end());
}

TEST(suggestion_iterator, rangeBased) {
  TempZimArchive tza("testZim");

  zim::Archive archive = tza.createZimFromContent({
    {"article a", "item a"},
    {"article b", "item b"},
    {"random c", "random c"}
  });

  zim::SuggestionSearcher searcher(archive);
  auto search = searcher.suggest("article");
  search.closeXapianIndex();    // Close xapian db to force rangeBased search

  ASSERT_EQ(search.getEstimatedMatches(), 2);
  auto srs = search.getResults(0, archive.getEntryCount());
  ASSERT_EQ(srs.size(), 2);

  auto it1 = srs.begin();
  ASSERT_EQ(it1->getTitle(), "article a");
  ASSERT_EQ(it1.getEntry().getPath(), "dummyPatharticle a");

  auto suggestionItem = *it1;
  ASSERT_FALSE(suggestionItem.hasSnippet());
  ASSERT_EQ(suggestionItem.getTitle(), "article a");

  zim::SuggestionIterator it2 = it1;
  ASSERT_EQ(it1->getTitle(), it2->getTitle());

  it1++;
  ASSERT_EQ(it1->getTitle(), "article b");
  it1--;
  ASSERT_EQ(it1->getTitle(), "article a");

  it2 = it1;
  ASSERT_TRUE(it2 == it1);

  it2 = srs.end();
  ASSERT_EQ(it2->getTitle(), "random c");
}

} // anonymous namespace
