/*
 * Copyright (C) 2020 Veloman Yunkan
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
#include <zim/item.h>
#include <zim/search.h>

#include "tools.h"

#include "tools.h"
#include "gtest/gtest.h"

namespace
{

using zim::unittests::getDataFilePath;
using zim::unittests::TempZimArchive;
using zim::unittests::TestItem;

std::vector<std::string> getSnippet(const zim::Archive archive, std::string query, int range) {
  zim::Search search(archive);
  search.set_suggestion_mode(false);
  search.set_query(query);
  search.set_range(0, range);
  search.set_verbose(true);

  std::vector<std::string> snippets;
  for (auto entry = search.begin(); entry != search.end(); entry++) {
    snippets.push_back(entry.get_snippet());
  }
  return snippets;
}

#define EXPECT_SNIPPET_EQ(archive, range, query, ...)           \
  ASSERT_EQ(                                                    \
    getSnippet(archive, query, range),                          \
    std::vector<std::string>({__VA_ARGS__})                     \
  )

#if WITH_TEST_DATA
TEST(Search, searchByTitle)
{
  for(auto& testfile:getDataFilePath("small.zim")) {
    const zim::Archive archive(testfile.path);
    ASSERT_TRUE(archive.hasTitleIndex());
    const auto mainItem = archive.getMainEntry().getItem(true);
    zim::Search search(archive);
    search.set_suggestion_mode(true);
    search.set_query(mainItem.getTitle());
    search.set_range(0, archive.getEntryCount());
    ASSERT_NE(0, search.get_matches_estimated());
    ASSERT_EQ(mainItem.getPath(), search.begin().get_path());
  }
}
#endif

// To secure compatibity of new zim files with older kiwixes, we need to index
// full path of the entries as data of documents.
TEST(Search, indexFullPath)
{
  TempZimArchive tza("testZim");
  zim::writer::Creator creator;
  creator.configIndexing(true, "en");
  creator.startZimCreation(tza.getPath());

  auto item = std::make_shared<TestItem>("testPath", "text/html", "Test Article", "This is a test article");
  creator.addItem(item);

  creator.setMainPath("testPath");
  creator.addMetadata("Title", "Test zim");
  creator.finishZimCreation();

  zim::Archive archive(tza.getPath());

  zim::Search search(archive);
  search.set_suggestion_mode(false);
  search.set_query("test article");
  search.set_range(0, archive.getEntryCount());
  search.set_verbose(true);

  ASSERT_EQ(search.begin().get_path(), "testPath");
  ASSERT_EQ(search.begin().get_dbData().substr(0, 2), "C/");
}

TEST(Search, fulltextSnippet)
{
  TempZimArchive tza("testZim");
  zim::writer::Creator creator;
  creator.configIndexing(true, "en");
  creator.startZimCreation(tza.getPath());

  auto item = std::make_shared<TestItem>("testPath", "text/html", "Test Article", "this is the content of a random paragraph without any context");
  creator.addItem(item);

  creator.setMainPath("testPath");
  creator.addMetadata("Title", "Test zim");
  creator.finishZimCreation();

  zim::Archive archive(tza.getPath());

  EXPECT_SNIPPET_EQ(
    archive,
    1,
    "random paragraph context",
    {
      "this is the content of a <b>random</b> <b>paragraph</b> without any <b>context</b>"
    }
  );
}

} // unnamed namespace
