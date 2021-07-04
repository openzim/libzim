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
#include <zim/item.h>

#include "tools.h"

#include "gtest/gtest.h"

namespace {

  using zim::unittests::TempZimArchive;
  using zim::unittests::TestItem;
  using zim::unittests::getDataFilePath;

  std::vector<std::string> getSuggestions(const zim::Archive archive, std::string query, int range) {
    zim::SuggestionSearcher suggestionSearcher(archive);
    zim::Query _query;
    _query.setQuery(query).setVerbose(true);
    auto suggestionSearch = suggestionSearcher.suggest(_query);

    auto suggestionResult = suggestionSearch.getResults(0, range);

    std::vector<std::string> result;
    for (auto entry : suggestionResult) {
      result.push_back(entry.getTitle());
    }
    return result;
  }

  std::vector<std::string> getSnippet(const zim::Archive archive, std::string query, int range) {
    zim::SuggestionSearcher suggestionSearcher(archive);
    zim::Query _query;
    _query.setQuery(query);
    auto suggestionSearch = suggestionSearcher.suggest(_query);
    auto result = suggestionSearch.getResults(0, range);

    std::vector<std::string> snippets;
    for (auto entry : result) {
      snippets.push_back(entry.getSnippet());
    }
    return snippets;
  }

#define EXPECT_SUGGESTION_RESULTS(archive, query, ...)          \
  ASSERT_EQ(                                                    \
      getSuggestions(archive, query, archive.getEntryCount()),  \
      std::vector<std::string>({__VA_ARGS__})                   \
  )

#define EXPECT_SNIPPET_EQ(archive, range, query, ...)           \
  ASSERT_EQ(                                                    \
    getSnippet(archive, query, range),                          \
    std::vector<std::string>({__VA_ARGS__})                     \
  )                                                             \

#if WITH_TEST_DATA
TEST(Suggestion, searchByTitle)
{
  for(auto& testfile:getDataFilePath("small.zim")) {
    const zim::Archive archive(testfile.path);
    ASSERT_TRUE(archive.hasTitleIndex());
    const auto mainItem = archive.getMainEntry().getItem(true);
    zim::SuggestionSearcher suggestionSearcher(archive);
    zim::Query query;
    query.setQuery(mainItem.getTitle());
    auto suggestionSearch = suggestionSearcher.suggest(query);
    ASSERT_NE(0, suggestionSearch.getEstimatedMatches());
    auto result = suggestionSearch.getResults(0, archive.getEntryCount());
    ASSERT_EQ(mainItem.getPath(), result.begin()->getPath());
  }
}
#endif


  TEST(Suggestion, emptyQuery) {
    std::vector<std::string> titles = {
                                        "fooland",
                                        "berlin wall",
                                        "hotel berlin, berlin",
                                        "again berlin",
                                        "berlin",
                                        "not berlin"
                                      };

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    std::vector<std::string> resultSet = getSuggestions(archive, "", archive.getEntryCount());
    std::vector<std::string> expectedResult = {};

    ASSERT_EQ(resultSet, expectedResult);
  }

  TEST(Suggestion, noResult) {
    std::vector<std::string> titles = {
                                        "fooland"
                                        "berlin wall",
                                        "hotel berlin, berlin",
                                        "again berlin",
                                        "berlin",
                                        "not berlin"
                                      };

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    std::vector<std::string> resultSet = getSuggestions(archive, "none", archive.getEntryCount());
    std::vector<std::string> expectedResult = {};

    ASSERT_EQ(resultSet, expectedResult);
  }

  TEST(Suggestion, singleTermOrder) {
    std::vector<std::string> titles = {
                                        "fooland",
                                        "berlin wall",
                                        "hotel berlin, berlin",
                                        "again berlin",
                                        "berlin",
                                        "not berlin"
                                      };

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    std::vector<std::string> resultSet = getSuggestions(archive, "berlin", archive.getEntryCount());
    std::vector<std::string> expectedResult = {
                                                "berlin",
                                                "berlin wall",
                                                "hotel berlin, berlin",
                                                "again berlin",
                                                "not berlin"
                                              };

    ASSERT_EQ(expectedResult , resultSet);
  }

  TEST(Suggestion, resultsGreaterThanLimit) {
    std::vector<std::string> titles = {
                                        "foobar b",
                                        "foobar a",
                                        "foobar c",
                                        "foobar e",
                                        "foobar d"
                                      };

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    std::vector<std::string> resultSet = getSuggestions(archive, "foobar", 2);
    std::vector<std::string> expectedResult = {
                                                "foobar a",
                                                "foobar b"
                                              };

    ASSERT_EQ(expectedResult, resultSet);
  }

  TEST(Suggestion, partialQuery) {
    std::vector<std::string> titles = {
                                        "The chocolate factory",
                                        "The wolf of Shingashina",
                                        "The wolf of Wall Street",
                                        "Hour of the wolf",
                                        "Wolf",
                                        "Terma termb the wolf of wall street termc"
                                      };

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    // "wo"
    std::vector<std::string> resultSet = getSuggestions(archive, "Wo", archive.getEntryCount());
    std::vector<std::string> expectedResult = {
                                                "Wolf",
                                                "Hour of the wolf",
                                                "The wolf of Shingashina",
                                                "The wolf of Wall Street",
                                                "Terma termb the wolf of wall street termc"
                                              };

    ASSERT_EQ(expectedResult, resultSet);
  }

  TEST(Suggestion, phraseOrder) {
    std::vector<std::string> titles = {
                                        "summer winter autumn",
                                        "winter autumn summer terma",
                                        "autumn summer winter",
                                        "control document",
                                        "summer",
                                      };

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    std::vector<std::string> resultSet = getSuggestions(archive, "winter autumn summer", archive.getEntryCount());
    std::vector<std::string> expectedResult = {
                                                "winter autumn summer terma",
                                                "autumn summer winter",
                                                "summer winter autumn"
                                              };

    ASSERT_EQ(expectedResult, resultSet);
  }

  TEST(Suggestion, incrementalSearch) {
    std::vector<std::string> titles = {
                                        "The chocolate factory",
                                        "The wolf of Shingashina",
                                        "The wolf of Wall Street",
                                        "The wolf among sheeps",
                                        "The wolf of Wall Street Book" ,
                                        "Hour of the wolf",
                                        "Wolf",
                                        "Terma termb the wolf of wall street termc"
                                      };

    std::vector<std::string> resultSet, expectedResult;

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    // "wolf"
    resultSet = getSuggestions(archive, "Wolf", archive.getEntryCount());
    expectedResult = {
                       "Wolf",
                       "Hour of the wolf",
                       "The wolf among sheeps",
                       "The wolf of Shingashina",
                       "The wolf of Wall Street",
                       "The wolf of Wall Street Book",
                       "Terma termb the wolf of wall street termc"
                     };

    ASSERT_EQ(expectedResult, resultSet);

    // "the"
    resultSet = getSuggestions(archive, "the", archive.getEntryCount());
    expectedResult = {
                       "The chocolate factory",
                       "The wolf among sheeps",
                       "The wolf of Shingashina",
                       "The wolf of Wall Street",
                       "The wolf of Wall Street Book",
                       "Hour of the wolf",
                       "Terma termb the wolf of wall street termc"
                     };

    ASSERT_EQ(expectedResult, resultSet);

    // "the wolf"
    resultSet = getSuggestions(archive, "the wolf", archive.getEntryCount());
    expectedResult = {
                       "The wolf among sheeps",
                       "The wolf of Shingashina",
                       "The wolf of Wall Street",
                       "The wolf of Wall Street Book",
                       "Hour of the wolf",
                       "Terma termb the wolf of wall street termc"
                     };

    ASSERT_EQ(expectedResult, resultSet);

    // "the wolf of"
    resultSet = getSuggestions(archive, "the wolf of", archive.getEntryCount());
    expectedResult = {
                       "The wolf of Shingashina",
                       "The wolf of Wall Street",
                       "The wolf of Wall Street Book",
                       "Terma termb the wolf of wall street termc",
                       "Hour of the wolf"
                     };

    ASSERT_EQ(expectedResult, resultSet);

    // "the wolf of wall"
    resultSet = getSuggestions(archive, "the wolf of wall", archive.getEntryCount());
    expectedResult = {
                       "The wolf of Wall Street",
                       "The wolf of Wall Street Book",
                       "Terma termb the wolf of wall street termc"
                     };

    ASSERT_EQ(expectedResult, resultSet);
  }

  TEST(Suggestion, phraseOutOfWindow) {
    std::vector<std::string> titles = {
                                        "This query",
                                        "This is the dummy query phrase",
                                        "the aterm bterm dummy cterm query",
                                        "aterm the bterm dummy query cterm"
                                      };

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    std::vector<std::string> resultSet = getSuggestions(archive, "the dummy query", archive.getEntryCount());
    std::vector<std::string> expectedResult = {
                                                "This is the dummy query phrase",
                                                "aterm the bterm dummy query cterm",
                                                "the aterm bterm dummy cterm query"
                                              };

    ASSERT_EQ(expectedResult, resultSet);
  }

  TEST(Suggestion, checkStopword) {
    std::vector<std::string> titles = {
                                        "she and the apple",
                                        "apple",
                                        "she and the"
                                      };

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    // "she", "and", "the" are stopwords, If stopwords are properly handled, they
    // should be included in the result documents.
    std::vector<std::string> resultSet = getSuggestions(archive, "she and the apple", archive.getEntryCount());
    std::vector<std::string> expectedResult = {
                                                "she and the apple"
                                              };
    ASSERT_EQ(expectedResult, resultSet);
  }

  TEST(Suggestion, checkRedirectionCollapse) {
    TempZimArchive tza("testZim");
    zim::writer::Creator creator;
    creator.configIndexing(true, "en");
    creator.startZimCreation(tza.getPath());

    auto item = std::make_shared<TestItem>("testPath", "text/html", "Article Target");
    creator.addItem(item);
    creator.addRedirection("redirectionPath1", "Article Redirect 1", "testPath");
    creator.addRedirection("redirectionPath2", "Article Redirect 2", "testPath");

    creator.addMetadata("Title", "Test zim");
    creator.finishZimCreation();

    zim::Archive archive(tza.getPath());
    std::vector<std::string> resultSet = getSuggestions(archive, "Article", archive.getEntryCount());

    // We should get only one result
    std::vector<std::string> expectedResult = {
                                                "Article Target",
                                              };
    ASSERT_EQ(resultSet, expectedResult);
  }

  TEST(Suggestion, checkRedirectionChain) {
    /*
     * As of now, we do not handle redirection chain. So if we have articles such
     * as A->B->C. Even if A B and C are essentially the same articles, They won't
     * get collapsed as one.
     */
    TempZimArchive tza("testZim");
    zim::writer::Creator creator;
    creator.configIndexing(true, "en");
    creator.startZimCreation(tza.getPath());

    auto item = std::make_shared<TestItem>("testPath", "text/html", "Article Target");
    creator.addItem(item);
    creator.addRedirection("redirectionPath1", "Article Redirect 1", "testPath");
    creator.addRedirection("redirectionPath2", "Article Redirect 2", "redirectionPath1");

    creator.addMetadata("Title", "Test zim");
    creator.finishZimCreation();

    zim::Archive archive(tza.getPath());
    std::vector<std::string> resultSet = getSuggestions(archive, "Article", archive.getEntryCount());

    // We should get only one result
    std::vector<std::string> expectedResult = {
                                                "Article Target",
                                                "Article Redirect 2"
                                              };
    ASSERT_EQ(resultSet, expectedResult);
  }

  // Different articles with same title should not be collapsed in suggestions
  TEST(Suggestion, diffArticleSameTitle) {
    TempZimArchive tza("testZim");
    zim::writer::Creator creator;
    creator.configIndexing(true, "en");
    creator.startZimCreation(tza.getPath());

    auto item1 = std::make_shared<TestItem>("testPath1", "text/html", "Test Article");
    auto item2 = std::make_shared<TestItem>("testPath2", "text/html", "Test Article");
    creator.addItem(item1);
    creator.addItem(item2);

    creator.addMetadata("Title", "Test zim");
    creator.finishZimCreation();

    zim::Archive archive(tza.getPath());
    std::vector<std::string> resultSet = getSuggestions(archive, "Test Article", archive.getEntryCount());

    // We should get two results
    std::vector<std::string> expectedResult = {
                                                "Test Article",
                                                "Test Article"
                                              };
    ASSERT_EQ(resultSet, expectedResult);
  }

  // Titles which begins with the search string should have higher relevance
  TEST(Suggestion, anchorQueryToBeginning) {
    std::vector<std::string> titles = {
                                        "aterm bterm this is a title cterm",
                                        "this is a title aterm bterm cterm",
                                        "aterm this is a title bterm cterm"
                                      };

    TempZimArchive tza("testZim");
    const zim::Archive archive = tza.createZimFromTitles(titles);

    std::vector<std::string> resultSet = getSuggestions(archive, "This is a title", archive.getEntryCount());
    std::vector<std::string> expectedResult = {
                                                "this is a title aterm bterm cterm",
                                                "aterm bterm this is a title cterm",
                                                "aterm this is a title bterm cterm"
                                              };

    ASSERT_EQ(expectedResult, resultSet);
  }

  // To secure compatibity of new zim files with older kiwixes, we need to index
  // full path of the entries as data of documents.
  TEST(Suggestion, indexFullPath) {
    TempZimArchive tza("testZim");
    zim::writer::Creator creator;
    creator.configIndexing(true, "en");
    creator.startZimCreation(tza.getPath());

    auto item = std::make_shared<TestItem>("testPath", "text/html", "Test Article");
    creator.addItem(item);

    creator.addMetadata("Title", "Test zim");
    creator.finishZimCreation();

    zim::Archive archive(tza.getPath());

    zim::SuggestionSearcher suggestionSearcher(archive);
    zim::Query query;
    query.setQuery("Test Article");
    auto suggestionSearch = suggestionSearcher.suggest(query);
    auto result = suggestionSearch.getResults(0, archive.getEntryCount());

    ASSERT_EQ(result.begin()->getPath(), "testPath");
    ASSERT_EQ(result.begin().getDbData().substr(0, 2), "C/");
  }

  TEST(Suggestion, nonWordCharacters) {
    TempZimArchive tza("testZim");
    {
      const zim::Archive archive = tza.createZimFromTitles({
        "Alice Bob",
        "Bonnie + Clyde",
        "Jack & Jill, on the hill"
      });

      EXPECT_SUGGESTION_RESULTS(archive, "Alice & Bob",
        "Alice Bob"
      );

      EXPECT_SUGGESTION_RESULTS(archive, "Bonnie + Clyde",
        "Bonnie + Clyde"
      );

      EXPECT_SUGGESTION_RESULTS(archive, "Jack & Jill",
        "Jack & Jill, on the hill"
      );
    }
  }

  TEST(Suggestion, titleSnippet) {
    TempZimArchive tza("testzim");

    const zim::Archive archive = tza.createZimFromTitles({
      "this is a straight run of matching words",
      "this is a broken set of likely words",
      "this is a long title to ensure that the snippets generated contain the entire title even if match is one word"
    });

    EXPECT_SNIPPET_EQ(
      archive,
      1,
      "straight run of matching",
      {
        "this is a <b>straight</b> <b>run</b> <b>of</b> <b>matching</b> words"
      }
    );

    EXPECT_SNIPPET_EQ(
      archive,
      1,
      "broken likely",
      {
        "this is a <b>broken</b> set of <b>likely</b> words"
      }
    );

    EXPECT_SNIPPET_EQ(
      archive,
      1,
      "generated",
      {
        "this is a long title to ensure that the snippets <b>generated</b> contain the entire title even if match is one word"
      }
    );

    EXPECT_SNIPPET_EQ(
      archive,
      archive.getEntryCount(),
      "this is",
      {
        "<b>this</b> <b>is</b> a broken set of likely words",
        "<b>this</b> <b>is</b> a straight run of matching words",
        "<b>this</b> <b>is</b> a long title to ensure that the snippets generated contain the entire title even if match <b>is</b> one word"
      }
    );
  }
}
