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

#include <xapian.h>

#include "tools.h"
#include "gtest/gtest.h"

namespace
{

using zim::unittests::TempZimArchive;
using zim::unittests::TestItem;

std::vector<std::string> getSnippet(const zim::Archive archive, std::string query, int range) {
  zim::Searcher searcher(archive);
  zim::Query _query;
  _query.setQuery(query);
  auto search = searcher.search(_query);
  auto result = search.getResults(0, range);

  std::vector<std::string> snippets;
  for (auto entry = result.begin(); entry != result.end(); entry++) {
    snippets.push_back(entry.getSnippet());
  }
  return snippets;
}

#define EXPECT_SNIPPET_EQ(archive, range, query, ...)           \
  ASSERT_EQ(                                                    \
    getSnippet(archive, query, range),                          \
    std::vector<std::string>({__VA_ARGS__})                     \
  )

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

  zim::Searcher searcher(archive);
  zim::Query query;
  query.setQuery("test article");
  auto search = searcher.search(query);

  ASSERT_NE(0, search.getEstimatedMatches());
  auto result = search.getResults(0, archive.getEntryCount());
  ASSERT_EQ(result.begin().getPath(), "testPath");
  ASSERT_EQ(result.begin().getDbData().substr(0, 2), "C/");
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

TEST(Search, multiSearch)
{
  TempZimArchive tza("testZim");

  zim::writer::Creator creator;
  creator.configIndexing(true, "en");
  creator.startZimCreation(tza.getPath());
  creator.addItem(std::make_shared<TestItem>("path0", "text/html", "Test Article0", "This is a test article. temp0"));
  creator.addItem(std::make_shared<TestItem>("path1", "text/html", "Test Article1", "This is another test article. For article1."));
  creator.addItem(std::make_shared<TestItem>("path2", "text/html", "Test Article001", "This is a test article. Super. temp0"));
  creator.addItem(std::make_shared<TestItem>("path3", "text/html", "Test Article2", "This is a test article. Super."));
  creator.addItem(std::make_shared<TestItem>("path4", "text/html", "Test Article23", "This is a test article. bis."));

  creator.setMainPath("path0");
  creator.finishZimCreation();

  zim::Archive archive(tza.getPath());

  zim::Searcher searcher(archive);
  searcher.setVerbose(true);
  zim::Query query;
  query.setQuery("test article");
  auto search0 = searcher.search(query);

  ASSERT_EQ(archive.getEntryCount(), search0.getEstimatedMatches());
  auto result0 = search0.getResults(0, 2);
  ASSERT_EQ(result0.size(), 2);
  auto it0 = result0.begin();

  auto result1 = search0.getResults(0, 5);
  ASSERT_EQ(result1.size(), 5);
  auto it1 = result1.begin();

  ASSERT_EQ(it0.getPath(), it1.getPath());
  it0++; it1++;
  ASSERT_EQ(it0.getPath(), it1.getPath());
  it0++; it1++;
  ASSERT_EQ(it0, result0.end());
  it1++;it1++;it1++;
  ASSERT_EQ(it1, result1.end());

  // Check result retrieval in start ranges
  auto result2 = search0.getResults(0, 3);    // Should return 3 results
  ASSERT_EQ(result2.size(), 3);

  // Check result retrieval in middle ranges
  auto result3 = search0.getResults(2, 3);    // Should Return 3 result
  ASSERT_EQ(result3.size(), 3);

  // Be able to do a different search using the same searcher.
  query.setQuery("super");
  auto search1 = searcher.search(query);
  ASSERT_EQ(2, search1.getEstimatedMatches());

  auto searcher2(searcher);
  searcher2.setVerbose(true);
  query.setQuery("temp0");
  auto search2 = searcher2.search(query);
  auto result = search2.getResults(0, search2.getEstimatedMatches());
  ASSERT_EQ(2, search2.getEstimatedMatches());
  ASSERT_EQ(2, result.size());
}

TEST(Search, noFTIndex)
{
  TempZimArchive tza("testZim");

  zim::writer::Creator creator;
  creator.configIndexing(false, "en");
  creator.startZimCreation(tza.getPath());
  creator.addItem(std::make_shared<TestItem>("path0", "text/html", "Test Article0", "This is a test article. temp0"));

  creator.setMainPath("path0");
  creator.finishZimCreation();

  zim::Archive archive(tza.getPath());

  zim::Searcher searcher(archive);
  searcher.setVerbose(true);
  zim::Query query;
  query.setQuery("test article");
  ASSERT_THROW(searcher.search(query), std::runtime_error);
}

TEST(Search, noStemming)
{
  TempZimArchive tza("testZim");

  zim::writer::Creator creator;
  creator.configIndexing(true, "nostem");
  creator.startZimCreation(tza.getPath());
  creator.addItem(std::make_shared<TestItem>("path0", "text/html", "Test Article0", "This is a test article. temp0"));
  creator.addItem(std::make_shared<TestItem>("path1", "text/html", "Test Article1", "This is another test article. For article1."));

  creator.setMainPath("path0");
  creator.finishZimCreation();

  zim::Archive archive(tza.getPath());

  zim::Searcher searcher(std::vector<zim::Archive>{});
  searcher.add_archive(archive);
  searcher.setVerbose(true);

  zim::Query query;
  query.setQuery("test article");
  auto search = searcher.search(query);

  ASSERT_EQ(archive.getEntryCount(), search.getEstimatedMatches());
  auto result = search.getResults(0, 1);
  ASSERT_EQ(result.begin().getTitle(), "Test Article0");
}

TEST(Search, geoQuery)
{
  TempZimArchive tza("testZim");

  std::string content = R"(<html><head><meta name="keywords" content="some keyword important"><meta name="geo.position" content="45.000;10.000"></head><body>Test geoquery</body><html>)";
  zim::writer::Creator creator;
  creator.configIndexing(true, "en");
  creator.startZimCreation(tza.getPath());
  creator.addItem(std::make_shared<TestItem>("path0", "text/html", "Test Article", content));

  creator.setMainPath("path0");
  creator.finishZimCreation();

  zim::Archive archive(tza.getPath());

  zim::Searcher searcher(archive);
  searcher.setVerbose(true);

  zim::Query query;
  query.setQuery("geoquery");
  query.setGeorange(45.000, 10.000, 100);
  auto search = searcher.search(query);

  ASSERT_EQ(archive.getEntryCount(), search.getEstimatedMatches());
  auto result = search.getResults(0, 1);
  ASSERT_EQ(result.begin().getTitle(), "Test Article");
}
} // unnamed namespace
