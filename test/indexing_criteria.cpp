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

#define ZIM_PRIVATE
#include <zim/archive.h>
#include <zim/item.h>
#include <zim/search.h>
#include <zim/suggestion.h>
#include <zim/writer/item.h>

#include "tools.h"
#include "gtest/gtest.h"

namespace
{

using zim::unittests::TempZimArchive;
using zim::unittests::TestItem;
using zim::unittests::IsFrontArticle;


class TestIndexData : public zim::writer::IndexData {
  public:
    TestIndexData(const std::string& content)
      : m_content(content)
    {}

    bool hasIndexData() const { return ! m_content.empty(); }
    std::string getTitle() const { return ""; }
    std::string getContent() const { return m_content; }
    std::string getKeywords() const { return ""; }
    uint32_t getWordCount() const { return 1; }
    IndexData::GeoPosition getGeoPosition() const { return std::make_tuple(false, 0.0, 0.0); }

  private:
    std::string m_content;
};

class IndexDataItem : public TestItem {
  public:
    IndexDataItem(const std::string& path, const std::string& mimetype, const std::string& title, const std::string& content, std::shared_ptr<zim::writer::IndexData> indexData)
    : TestItem(path, mimetype, title, content),
      mp_indexData(indexData)
    {}

    std::shared_ptr<zim::writer::IndexData> getIndexData() const { return mp_indexData; }
  private:
    std::shared_ptr<zim::writer::IndexData> mp_indexData;
};

#if defined(ENABLE_XAPIAN)

TEST(IndexCriteria, defaultIndexingBaseOnMimeType)
{
  TempZimArchive tza("testZim");
  zim::writer::Creator creator;
  creator.configIndexing(true, "en");
  creator.startZimCreation(tza.getPath());

  creator.addItem(
    std::make_shared<TestItem>("HtmlTestPath", "text/html", "Test Article", "This is a test article")
  );

  creator.addItem(
    std::make_shared<TestItem>("OtherTestPath", "text/plain", "Test Article", "This is a test article")
  );
  creator.finishZimCreation();

  zim::Archive archive(tza.getPath());

  zim::Searcher searcher(archive);
  zim::Query query("test article");
  auto search = searcher.search(query);

  ASSERT_EQ(1, search.getEstimatedMatches());
  auto result = search.getResults(0, archive.getEntryCount());
  auto begin = result.begin();
  ASSERT_EQ(begin.getPath(), "HtmlTestPath");
  ASSERT_EQ(++begin, result.end());
}

TEST(IndexCriteria, specificIndexData)
{
  TempZimArchive tza("testZim");
  zim::writer::Creator creator;
  creator.configIndexing(true, "en");
  creator.startZimCreation(tza.getPath());

  // Html content is indexed by default
  creator.addItem(
    std::make_shared<TestItem>("HtmlTestPath", "text/html", "Test Article", "This is a test article")
  );

  // Non html content is not indexed by default
  creator.addItem(
    std::make_shared<TestItem>("OtherTestPath", "text/plain", "Test Article", "This is a test article")
  );

  // Item without a IndexData is not indexed
  creator.addItem(
    std::make_shared<IndexDataItem>("HtmlTestPathNull", "text/html", "Test Article", "This is a test article", nullptr)
  );

  // Item with a IndexData but without data is not indexed
  creator.addItem(
    std::make_shared<IndexDataItem>("HtmlTestPathNodata", "text/html", "Test Article", "This is a test article",
      std::make_shared<TestIndexData>(""))
  );

  // We index the content with the data of the indexdata if provided
  creator.addItem(
    std::make_shared<IndexDataItem>("OtherTestPathWithIndex", "text/plain", "Test Article", "This is content",
      std::make_shared<TestIndexData>("test article"))
  );
  creator.finishZimCreation();

  zim::Archive archive(tza.getPath());

  zim::Searcher searcher(archive);
  zim::Query query("test article");
  auto search = searcher.search(query);

  ASSERT_EQ(2, search.getEstimatedMatches());
  auto result = search.getResults(0, archive.getEntryCount());
  auto begin = result.begin();
  ASSERT_EQ(begin.getPath(), "HtmlTestPath");
  begin++;
  ASSERT_EQ(begin.getPath(), "OtherTestPathWithIndex");
  ASSERT_EQ(++begin, result.end());
}

#endif // ENABLE_XAPIAN

TEST(IndexCriteria, suggestion) {
  TempZimArchive tza("testZim");
  zim::writer::Creator creator;

  creator.startZimCreation(tza.getPath());

  // Default html is title indexed
  creator.addItem(
    std::make_shared<TestItem>("HtmlTestPath", "text/html", "Test Article", "This is a test article")
  );

  // Default not html is not title indexed
  creator.addItem(
    std::make_shared<TestItem>("OtherTestPath", "text/plain", "Test Article", "This is a test article")
  );

  // Default redirection is not indexed (even if pointing to html content)
  creator.addRedirection("Aredirect", "Test Article Redirection", "HtmlTestPath");

  // We can force a html content to not be title indexed
  creator.addItem(
    std::make_shared<TestItem>("HtmlTestPathForced", "text/html", "Test Article", "This is a test article", IsFrontArticle::NO)
  );

  // Default not html is not title indexed
  creator.addItem(
    std::make_shared<TestItem>("OtherTestPathForced", "text/plain", "Test Article", "This is a test article", IsFrontArticle::YES)
  );

  // Redirection need to point to something not already indexed.
  // As we collapse the suggestion by target path, if we have a redirection to a indexed entry,
  // the suggestion result will contain only one of them.
  creator.addRedirection("AredirectForced", "Test Article Redirection", "OtherTestPath", {{zim::writer::FRONT_ARTICLE, 1}});

  creator.finishZimCreation();

  zim::Archive archive(tza.getPath());

  zim::SuggestionSearcher suggestionSearcher(archive);
  auto suggestion = suggestionSearcher.suggest("test article");

  ASSERT_EQ(3, suggestion.getEstimatedMatches());
  auto result = suggestion.getResults(0, archive.getEntryCount());
  auto begin = result.begin();
  ASSERT_EQ(begin->getPath(), "HtmlTestPath");
  begin++;
  ASSERT_EQ(begin->getPath(), "OtherTestPathForced");
  begin++;
  ASSERT_EQ(begin->getPath(), "AredirectForced");
  ASSERT_EQ(++begin, result.end());
}

} // unnamed namespace
