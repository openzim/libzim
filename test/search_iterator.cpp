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
#include "tools.h"

#include "gtest/gtest.h"

namespace {

using zim::unittests::TempZimArchive;

TEST(search_iterator, functions) {
  TempZimArchive tza("testZim");

  zim::Archive archive = tza.createZimFromTitles({
    "item a",
  });

  zim::Searcher searcher(archive);
  auto search = searcher.search(true);
  search.set_query("item");
  search.set_range(0, archive.getEntryCount());
  search.set_verbose(true);

  auto it = search.begin();

  // Test functions
  ASSERT_EQ(it.get_title(), "item a");
  ASSERT_EQ(it.get_path(), "dummyPathitem a");
  ASSERT_EQ(it.get_score(), 100);
  ASSERT_EQ(it.get_fileIndex(), 0);
  ASSERT_EQ(it.get_wordCount(), -1);            // Unimplemented
  ASSERT_EQ(it.get_size(), -1);                 // Unimplemented
}

TEST(search_iterator, iteration) {
  TempZimArchive tza("testZim");

  zim::Archive archive = tza.createZimFromTitles({
    "item a",
    "item b"
  });

  zim::Searcher searcher(archive);
  auto search = searcher.search(true);
  search.set_query("item");
  search.set_range(0, archive.getEntryCount());
  search.set_verbose(true);

  auto it = search.begin();
  ASSERT_EQ(it.get_title(), search.begin().get_title());

  ASSERT_EQ(it.get_title(), "item a");
  it++;
  ASSERT_EQ(it.get_title(), "item b");
  ASSERT_TRUE(it != search.begin());

  it--;
  ASSERT_EQ(it.get_title(), "item a");
  ASSERT_TRUE(search.begin() == it);

  it++; it++;
  ASSERT_TRUE(it == search.end());
}

} // anonymous namespace
