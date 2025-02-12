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
  suggestionSearcher.setVerbose(true);
  auto suggestionSearch = suggestionSearcher.suggest(query);
  auto suggestionResult = suggestionSearch.getResults(0, range);

  std::vector<std::string> result;
  for (auto entry : suggestionResult) {
    result.push_back(entry.getTitle());
  }
  return result;
}

std::vector<std::string> getSnippet(const zim::Archive archive, std::string query, int range) {
  zim::SuggestionSearcher suggestionSearcher(archive);
  auto suggestionSearch = suggestionSearcher.suggest(query);
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
    auto suggestionSearch = suggestionSearcher.suggest(mainItem.getTitle());
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

  EXPECT_SUGGESTION_RESULTS(archive, "berlin",
    "berlin",
    "berlin wall",
    "hotel berlin, berlin",
    "again berlin",
    "not berlin"
  );
}

TEST(Suggestion, caseDiacriticsAndHomogrpaphsHandling) {
  std::vector<std::string> titles = {
                                      "nonberlin",
                                      "simply berlin",
                                      "accented bérlin",
                                      "uppercase BERLIN",
                                      "homograph bеrlin", // е is cyrillic
                                    };

  TempZimArchive tza("testZim");
  const zim::Archive archive = tza.createZimFromTitles(titles);

  const std::vector<std::string> expectedResult{
                                                 "accented bérlin",
                                                 "simply berlin",
                                                 "uppercase BERLIN",
                                               };

  ASSERT_EQ(getSuggestions(archive, "berlin", archive.getEntryCount()),
            expectedResult
  );

  ASSERT_EQ(getSuggestions(archive, "BERLIN", archive.getEntryCount()),
            expectedResult
  );

  ASSERT_EQ(getSuggestions(archive, "bêřlïñ", archive.getEntryCount()),
            expectedResult
  );

  // е in the query string "bеrlin" below is a cyrillic character
  ASSERT_EQ(getSuggestions(archive, "bеrlin", archive.getEntryCount()),
            std::vector<std::string>{"homograph bеrlin"}
  );
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

  EXPECT_SUGGESTION_RESULTS(archive, "Wo",
    "Wolf",
    "Hour of the wolf",
    "The wolf of Shingashina",
    "The wolf of Wall Street",
    "Terma termb the wolf of wall street termc"
  );
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

  EXPECT_SUGGESTION_RESULTS(archive, "winter autumn summer",
    "winter autumn summer terma",
    "autumn summer winter",
    "summer winter autumn"
  );
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
                                      "Terma termb the wolf of wall street termc",
                                      "Are there any beasts in this country?"
                                    };

  TempZimArchive tza("testZim");
  const zim::Archive archive = tza.createZimFromTitles(titles);

  EXPECT_SUGGESTION_RESULTS(archive, "Wolf",
    "Wolf",
    "Hour of the wolf",
    "The wolf among sheeps",
    "The wolf of Shingashina",
    "The wolf of Wall Street",
    "The wolf of Wall Street Book",
    "Terma termb the wolf of wall street termc"
  );

  EXPECT_SUGGESTION_RESULTS(archive, "the",
    "The chocolate factory",
    "The wolf among sheeps",
    "The wolf of Shingashina",
    "The wolf of Wall Street",
    "The wolf of Wall Street Book",
    "Hour of the wolf",
    "Terma termb the wolf of wall street termc",
    "Are there any beasts in this country?"
  );

  EXPECT_SUGGESTION_RESULTS(archive, "the wol",
    "Hour of the wolf",
    "The wolf among sheeps",
    "The wolf of Shingashina",
    "The wolf of Wall Street",
    "The wolf of Wall Street Book",
    "Terma termb the wolf of wall street termc"
  );

  EXPECT_SUGGESTION_RESULTS(archive, "the wolf",
    "The wolf among sheeps",
    "The wolf of Shingashina",
    "The wolf of Wall Street",
    "The wolf of Wall Street Book",
    "Hour of the wolf",
    "Terma termb the wolf of wall street termc"
  );

  EXPECT_SUGGESTION_RESULTS(archive, "the wolf ",
    "The wolf among sheeps",
    "The wolf of Shingashina",
    "The wolf of Wall Street",
    "The wolf of Wall Street Book",
    "Hour of the wolf",
    "Terma termb the wolf of wall street termc"
  );

  EXPECT_SUGGESTION_RESULTS(archive, "the wolf of",
    "The wolf of Shingashina",
    "The wolf of Wall Street",
    "The wolf of Wall Street Book",
    "Terma termb the wolf of wall street termc",
    "Hour of the wolf"
  );

  EXPECT_SUGGESTION_RESULTS(archive, "the wolf of wall",
    "The wolf of Wall Street",
    "The wolf of Wall Street Book",
    "Terma termb the wolf of wall street termc"
  );
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

  EXPECT_SUGGESTION_RESULTS(archive, "the dummy query",
    "This is the dummy query phrase",
    "aterm the bterm dummy query cterm",
    "the aterm bterm dummy cterm query"
  );
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
  EXPECT_SUGGESTION_RESULTS(archive, "she and the apple",
    "she and the apple"
  );
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
  creator.addRedirection("redirectionPath2", "Article Redirect 2", "redirectionPath1", {{zim::writer::FRONT_ARTICLE, 1}});

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

  EXPECT_SUGGESTION_RESULTS(archive, "This is a title",
    "this is a title aterm bterm cterm",
    "aterm bterm this is a title cterm",
    "aterm this is a title bterm cterm"
  );
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
  auto suggestionSearch = suggestionSearcher.suggest("Test Article");
  auto result = suggestionSearch.getResults(0, archive.getEntryCount());

  ASSERT_EQ(result.begin()->getPath(), "testPath");
  ASSERT_EQ(result.begin().getDbData().substr(0, 2), "C/");
}

TEST(Suggestion, nonWordCharacters) {
  TempZimArchive tza("testZim");
  {
    const zim::Archive archive = tza.createZimFromTitles({
      "Alice Bob",
      "Alice & Bob",
      "Bonnie + Clyde",
      "Jack & Jill, on the hill",
      "Ali Baba & the 40 thieves",
      "&",
      "&%#"
    });

    // this test-point has nothing to do with the purpose of this unit-test
    // however I couldn't stand the temptation of adding it.
    EXPECT_SUGGESTION_RESULTS(archive, "Ali",
      "Ali Baba & the 40 thieves",
      "Alice & Bob",
      "Alice Bob",
    );

    EXPECT_SUGGESTION_RESULTS(archive, "Alice Bob",
      "Alice & Bob",
      "Alice Bob"
    );

    EXPECT_SUGGESTION_RESULTS(archive, "Alice & Bob",
      "Alice & Bob",
      "Alice Bob"
    );

    EXPECT_SUGGESTION_RESULTS(archive, "Bonnie + Clyde",
      "Bonnie + Clyde"
    );

    EXPECT_SUGGESTION_RESULTS(archive, "Jack & Jill",
      "Jack & Jill, on the hill"
    );

    EXPECT_SUGGESTION_RESULTS(archive, "4",
      "Ali Baba & the 40 thieves"
    );

    EXPECT_SUGGESTION_RESULTS(archive, "40",
      "Ali Baba & the 40 thieves"
    );

    EXPECT_SUGGESTION_RESULTS(archive, "&",
      "&",
      "&%#"
      // "Jack & Jill ...", "Alice & Bob" and "Ali Baba & the 40 thieves" not
      // included since inside those titles "&" is treated as noise.
    );

    EXPECT_SUGGESTION_RESULTS(archive, "&%#",
      "&%#"
    );
  }
}

TEST(Suggestion, TitlesMadeOfStopWordsOnly) {
  TempZimArchive tza("testZim");
  {
    const zim::Archive archive = tza.createZimFromTitles({
      "The",
      "Are you at home?",
      "Back and forth",
      "One, two, three...",
      "Not at all",
      "Do not act before you have to"
    });

    EXPECT_SUGGESTION_RESULTS(archive, "the",
        "The"
    );

    EXPECT_SUGGESTION_RESULTS(archive, "not",
        "Not at all",
        "Do not act before you have to"
    );

    EXPECT_SUGGESTION_RESULTS(archive, "at",
        "Not at all",
        "Are you at home?"
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

TEST(Suggestion, reuseSearcher) {
  std::vector<std::string> titles = {
                                      "song for you",
                                      "sing a song for you",
                                      "a song b for c you",
                                      "song for someone"
                                    };

  TempZimArchive tza("testZim");
  const zim::Archive archive = tza.createZimFromTitles(titles);

  zim::SuggestionSearcher suggestionSearcher(archive);
  suggestionSearcher.setVerbose(true);
  auto suggestionSearch1 = suggestionSearcher.suggest("song for you");
  auto suggestionResult1 = suggestionSearch1.getResults(0, 2);

  int count = 0;
  for (auto entry : suggestionResult1) {
    count++;
  }

  auto suggestionSearch2 = suggestionSearcher.suggest("song for you");
  auto suggestionResult2 = suggestionSearch2.getResults(2, archive.getEntryCount());

  for (auto entry : suggestionResult2) {
    count++;
  }
  ASSERT_EQ(count, 3);
}

TEST(Suggestion, CJK) {
  TempZimArchive tza("testZim");
  zim::writer::Creator creator;
  creator.configIndexing(true, "zh");
  creator.startZimCreation(tza.getPath());

  auto item1 = std::make_shared<TestItem>("testPath1", "text/html", "平方");
  auto item2 = std::make_shared<TestItem>("testPath2", "text/html", "平方根");
  creator.addItem(item1);
  creator.addItem(item2);

  creator.addMetadata("Title", "Test zim");
  creator.finishZimCreation();

  zim::Archive archive(tza.getPath());
  EXPECT_SUGGESTION_RESULTS(archive, "平方",
    "平方",
    "平方根"
  );

  EXPECT_SUGGESTION_RESULTS(archive, "平方根",
    "平方根"
  );
}

} // unnamed namespace
