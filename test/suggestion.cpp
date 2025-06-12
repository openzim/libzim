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

#include <xapian.h>

#include "tools.h"
#include "../src/tools.h"

#include "gtest/gtest.h"

namespace {

using zim::unittests::TempZimArchive;
using zim::unittests::TestItem;
using zim::unittests::getDataFilePath;

std::vector<std::string> getSuggestedTitles(const zim::Archive archive, std::string query, int range) {
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

#define EXPECT_SUGGESTED_TITLES(archive, query, ...)               \
  ASSERT_EQ(                                                       \
      getSuggestedTitles(archive, query, archive.getEntryCount()), \
      std::vector<std::string>({__VA_ARGS__})                      \
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

  std::vector<std::string> resultSet = getSuggestedTitles(archive, "", archive.getEntryCount());
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

  std::vector<std::string> resultSet = getSuggestedTitles(archive, "none", archive.getEntryCount());
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

  EXPECT_SUGGESTED_TITLES(archive, "berlin",
    "berlin",
    "berlin wall",
    "hotel berlin, berlin",
    "again berlin",
    "not berlin"
  );
}

TEST(Suggestion, caseDiacriticsAndHomographsHandling) {
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

  ASSERT_EQ(getSuggestedTitles(archive, "berlin", archive.getEntryCount()),
            expectedResult
  );

  ASSERT_EQ(getSuggestedTitles(archive, "BERLIN", archive.getEntryCount()),
            expectedResult
  );

  ASSERT_EQ(getSuggestedTitles(archive, "bêřlïñ", archive.getEntryCount()),
            expectedResult
  );

  // е in the query string "bеrlin" below is a cyrillic character
  ASSERT_EQ(getSuggestedTitles(archive, "bеrlin", archive.getEntryCount()),
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

  std::vector<std::string> resultSet = getSuggestedTitles(archive, "foobar", 2);
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

  EXPECT_SUGGESTED_TITLES(archive, "Wo",
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

  EXPECT_SUGGESTED_TITLES(archive, "winter autumn summer",
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

  EXPECT_SUGGESTED_TITLES(archive, "Wolf",
    "Wolf",
    "Hour of the wolf",
    "The wolf among sheeps",
    "The wolf of Shingashina",
    "The wolf of Wall Street",
    "The wolf of Wall Street Book",
    "Terma termb the wolf of wall street termc"
  );

  EXPECT_SUGGESTED_TITLES(archive, "Wolf ",
    "Wolf",
    "Hour of the wolf",
    "The wolf among sheeps",
    "The wolf of Shingashina",
    "The wolf of Wall Street",
    "The wolf of Wall Street Book",
    "Terma termb the wolf of wall street termc"
  );

  EXPECT_SUGGESTED_TITLES(archive, "the",
    "The chocolate factory",
    "The wolf among sheeps",
    "The wolf of Shingashina",
    "The wolf of Wall Street",
    "The wolf of Wall Street Book",
    "Hour of the wolf",
    "Terma termb the wolf of wall street termc",
    "Are there any beasts in this country?"
  );

  EXPECT_SUGGESTED_TITLES(archive, "the ",
    "The chocolate factory",
    "The wolf among sheeps",
    "The wolf of Shingashina",
    "The wolf of Wall Street",
    "The wolf of Wall Street Book",
    "Hour of the wolf",
    "Terma termb the wolf of wall street termc"
  );

  EXPECT_SUGGESTED_TITLES(archive, "the wol",
    "Hour of the wolf",
    "The wolf among sheeps",
    "The wolf of Shingashina",
    "The wolf of Wall Street",
    "The wolf of Wall Street Book",
    "Terma termb the wolf of wall street termc"
  );

  EXPECT_SUGGESTED_TITLES(archive, "the wolf",
    "The wolf among sheeps",
    "The wolf of Shingashina",
    "The wolf of Wall Street",
    "The wolf of Wall Street Book",
    "Hour of the wolf",
    "Terma termb the wolf of wall street termc"
  );

  EXPECT_SUGGESTED_TITLES(archive, "the wolf ",
    "The wolf among sheeps",
    "The wolf of Shingashina",
    "The wolf of Wall Street",
    "The wolf of Wall Street Book",
    "Hour of the wolf",
    "Terma termb the wolf of wall street termc"
  );

  EXPECT_SUGGESTED_TITLES(archive, "the wolf of",
    "The wolf of Shingashina",
    "The wolf of Wall Street",
    "The wolf of Wall Street Book",
    "Terma termb the wolf of wall street termc",
    "Hour of the wolf"
  );

  EXPECT_SUGGESTED_TITLES(archive, "the wolf of wall",
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

  EXPECT_SUGGESTED_TITLES(archive, "the dummy query",
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
  EXPECT_SUGGESTED_TITLES(archive, "she and the apple",
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
  std::vector<std::string> resultSet = getSuggestedTitles(archive, "Article", archive.getEntryCount());

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
  std::vector<std::string> resultSet = getSuggestedTitles(archive, "Article", archive.getEntryCount());

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
  std::vector<std::string> resultSet = getSuggestedTitles(archive, "Test Article", archive.getEntryCount());

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

  EXPECT_SUGGESTED_TITLES(archive, "This is a title",
    "this is a title aterm bterm cterm",
    "aterm bterm this is a title cterm",
    "aterm this is a title bterm cterm"
  );
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
    EXPECT_SUGGESTED_TITLES(archive, "Ali",
      "Ali Baba & the 40 thieves",
      "Alice & Bob",
      "Alice Bob",
    );

    EXPECT_SUGGESTED_TITLES(archive, "Alice Bob",
      "Alice & Bob",
      "Alice Bob"
    );

    EXPECT_SUGGESTED_TITLES(archive, "Alice & Bob",
      "Alice & Bob",
      "Alice Bob"
    );

    EXPECT_SUGGESTED_TITLES(archive, "Bonnie + Clyde",
      "Bonnie + Clyde"
    );

    EXPECT_SUGGESTED_TITLES(archive, "Jack & Jill",
      "Jack & Jill, on the hill"
    );

    EXPECT_SUGGESTED_TITLES(archive, "4",
      "Ali Baba & the 40 thieves"
    );

    EXPECT_SUGGESTED_TITLES(archive, "40",
      "Ali Baba & the 40 thieves"
    );

    EXPECT_SUGGESTED_TITLES(archive, "&",
      "&",
      "&%#"
      // "Jack & Jill ...", "Alice & Bob" and "Ali Baba & the 40 thieves" not
      // included since inside those titles "&" is treated as noise.
    );

    EXPECT_SUGGESTED_TITLES(archive, "&%#",
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

    EXPECT_SUGGESTED_TITLES(archive, "the",
        "The"
    );

    EXPECT_SUGGESTED_TITLES(archive, "not",
        "Not at all",
        "Do not act before you have to"
    );

    EXPECT_SUGGESTED_TITLES(archive, "at",
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

std::shared_ptr<TestItem> makeHtmlItem(std::string path, std::string title) {
  return std::make_shared<TestItem>(path, "text/html", title);
}

class TempZimArchiveMadeOfEmptyHtmlArticles
{
  using PathAndTitle = std::pair<std::string, std::string>;

public:
  TempZimArchiveMadeOfEmptyHtmlArticles(std::string lang,
                                        const std::vector<PathAndTitle>& data)
    : tza("testZim")
  {
    zim::writer::Creator creator;
    creator.configIndexing(true, lang);
    creator.startZimCreation(tza.getPath());

    for ( const auto& pathAndTile : data ) {
      creator.addItem(makeHtmlItem(pathAndTile.first, pathAndTile.second));
    }

    creator.addMetadata("Title", "Test zim");
    creator.finishZimCreation();
  }

  std::string getPath() const { return tza.getPath(); }

private:
  TempZimArchive tza;
};

TEST(Suggestion, CJK) {
  TempZimArchiveMadeOfEmptyHtmlArticles tza("zh", {
       //  path     , title
       { "testPath1", "平方"   },
       { "testPath2", "平方根" },
  });

  zim::Archive archive(tza.getPath());
  EXPECT_SUGGESTED_TITLES(archive, "平方",
    "平方",
    "平方根"
  );

  EXPECT_SUGGESTED_TITLES(archive, "平方根",
    "平方根"
  );
}

TEST(Suggestion, titleEdgeCases) {
  TempZimArchiveMadeOfEmptyHtmlArticles tza("en", {
     // { path     , title   }

        { "About"  , "About" }, // Title identical to path
        { "Trout"  , "trout" }, // Title differing from path in case only
        { "Without", ""      }, // No title

        // Non edge cases
        { "Stout",   "About Rex Stout" },
        { "Hangout", "Without a trout" },
  });

  zim::Archive archive(tza.getPath());
  EXPECT_SUGGESTED_TITLES(archive, "abo",
    "About",
    "About Rex Stout",
  );

  EXPECT_SUGGESTED_TITLES(archive, "witho",
    "Without", // this is a path rather than a title
    "Without a trout",
  );

  EXPECT_SUGGESTED_TITLES(archive, "tro",
    "trout",
    "Without a trout",
  );

  EXPECT_SUGGESTED_TITLES(archive, "hang"
      /* nothing */
  );
}

using SuggestionTuple = std::tuple<std::string, std::string, std::string>;

std::vector<std::string> getCompletionSuggestions(const zim::Archive& archive,
                                                  std::string query,
                                                  int maxSuggestions) {
  zim::SuggestionSearcher suggestionSearcher(archive);
  suggestionSearcher.setVerbose(true);
  const auto search = suggestionSearcher.suggest(query);
  std::vector<std::string> result;
  for (const auto& s : search.getAutocompletionSuggestions(maxSuggestions)) {
    EXPECT_EQ(s.getTitle(), "");
    EXPECT_EQ(s.getPath(), "");
    result.push_back(s.getSnippet());
  }
  return result;
}

#define EXPECT_COMPLETION_SUGGESTION_RESULTS(archive, query, maxSuggestions, parenthesizedExpectedResult) \
  ASSERT_EQ(                                                                   \
      getCompletionSuggestions(archive, query, maxSuggestions),                \
      std::vector<std::string> parenthesizedExpectedResult                     \
  )

TEST(Suggestion, autocompletionSuggestions) {
  TempZimArchive tza("testZim");
  const zim::Archive archive = tza.createZimFromTitles({
      "Hebrew for zebras in 24 hours",
      "Ebook formats",
      "Selling on EBay for mummies",
      "Patient Zero: the horrible story of ebola",
      "Bank erosion in Zimbabwe",
      "Error correcting codes",
      "Zimbabwe patent #19539",
      "All the king's horses",
      "Martin Luther King Jr.",
      "King Kong (1933 film)",
      "King-fu Panda",
      "Forrest Gump",
      "Ebay, Alibaba & the Forty Thieves",
      "Crazy Horse (disambiguation)"
  });

  EXPECT_COMPLETION_SUGGESTION_RESULTS(archive, "", 10, ({
  }));

  EXPECT_COMPLETION_SUGGESTION_RESULTS(archive, "asdf ", 10, ({
  }));

  // no completions for a single letter
  EXPECT_COMPLETION_SUGGESTION_RESULTS(archive, "k", 10, ({
  }));

  // no completions for a single letter
  EXPECT_COMPLETION_SUGGESTION_RESULTS(archive, "asdf k", 10, ({
  }));

  EXPECT_COMPLETION_SUGGESTION_RESULTS(archive, "ki", 10, ({
    "<b>king</b>",
    "<b>king's</b>", // XXX: possesive form
    /*"<b>king-fu</b>", */ // missing
  }));

  EXPECT_COMPLETION_SUGGESTION_RESULTS(archive, "Ki", 10, ({
    "<b>king</b>",
    "<b>king's</b>", // XXX: possesive form
    /*"<b>king-fu</b>", */ // missing
  }));

  EXPECT_COMPLETION_SUGGESTION_RESULTS(archive, "pa", 10, ({
    "<b>panda</b>",
    "<b>patent</b>",
    "<b>patient</b>",
  }));

  EXPECT_COMPLETION_SUGGESTION_RESULTS(archive, "pâ", 10, ({
    "<b>panda</b>",   // XXX: diacritics in the query are ignored
    "<b>patent</b>",  // XXX: diacritics in the query are ignored
    "<b>patient</b>", // XXX: diacritics in the query are ignored
  }));

  EXPECT_COMPLETION_SUGGESTION_RESULTS(archive, "patient", 10, ({
    "<b>patient</b>", // XXX: useless completion
  }));

  EXPECT_COMPLETION_SUGGESTION_RESULTS(archive, "19", 10, ({
    "<b>1933</b>",  // XXX: non word
    "<b>19539</b>", // XXX: non word
  }));

  EXPECT_COMPLETION_SUGGESTION_RESULTS(archive, "ze", 10, ({
    "<b>zebras</b>", // XXX: plural form
    "<b>zero</b>",
  }));

  EXPECT_COMPLETION_SUGGESTION_RESULTS(archive, "Ze", 10, ({
    "<b>zebras</b>", // XXX: plural form
    "<b>zero</b>",
  }));

  EXPECT_COMPLETION_SUGGESTION_RESULTS(archive, "fo", 10, ({
    "<b>for</b>",     // XXX: stop word
    "<b>formats</b>", // XXX: plural form
    "<b>forrest</b>",
    "<b>forty</b>",
  }));

  EXPECT_COMPLETION_SUGGESTION_RESULTS(archive, "ho", 10, ({
    "<b>horrible</b>",
    "<b>horse</b>",
    "<b>horses</b>", // XXX: plural form in addition to singular form above
    "<b>hours</b>",  // XXX: plural form
  }));

  EXPECT_COMPLETION_SUGGESTION_RESULTS(archive, "asdf pa", 10, ({
    "asdf <b>panda</b>",
    "asdf <b>patent</b>",
    "asdf <b>patient</b>",
  }));
}

std::vector<std::string> getSpellingSuggestions(const zim::Archive& archive,
                                                std::string query,
                                                int maxSuggestions) {
  zim::SuggestionSearcher suggestionSearcher(archive);
  suggestionSearcher.setVerbose(true);
  const auto search = suggestionSearcher.suggest(query);
  std::vector<std::string> result;
  for (const auto& s : search.getSpellingSuggestions(maxSuggestions)) {
    EXPECT_EQ(s.getTitle(), "");
    EXPECT_EQ(s.getPath(), "");
    result.push_back(s.getSnippet());
  }
  return result;
}

#define EXPECT_SPELLING_SUGGESTION_RESULTS(archive, query, maxSuggestions, parenthesizedExpectedResult) \
  ASSERT_EQ(                                                                   \
      getSpellingSuggestions(archive, query, maxSuggestions),                  \
      std::vector<std::string> parenthesizedExpectedResult                     \
  )

TEST(Suggestion, spellingSuggestions) {
  TempZimArchive tza("testZim");
  const zim::Archive archive = tza.createZimFromTitles({
      "Hebrew for zebras in 24 hours",
      "Patient Zero: the horrible story of ebola",
      "Zimbabwe patent #15539",
      "All the king's horses",
      "Martin Luther King Jr.",
      "King Kong (1933 film)",
      "King-fu Panda",
  });

  EXPECT_SPELLING_SUGGESTION_RESULTS(archive, "", 10, ({
  }));

  EXPECT_SPELLING_SUGGESTION_RESULTS(archive, "kung", 10, ({
    "<b>king</b>",
    "<b>kong</b>",
  }));

  EXPECT_SPELLING_SUGGESTION_RESULTS(archive, "song", 10, ({
    "<b>kong</b>",
    "<b>king</b>",
  }));

  EXPECT_SPELLING_SUGGESTION_RESULTS(archive, "wing", 10, ({
    "<b>king</b>",
    "<b>kong</b>",
  }));

  EXPECT_SPELLING_SUGGESTION_RESULTS(archive, "hourse", 10, ({
    "<b>hours</b>",
    "<b>horses</b>",
  }));

  EXPECT_SPELLING_SUGGESTION_RESULTS(archive, "hebras", 10, ({
    "<b>zebras</b>",
    "<b>hebrew</b>",
  }));

  EXPECT_SPELLING_SUGGESTION_RESULTS(archive, "paient", 10, ({
    "<b>patent</b>",
    "<b>patient</b>",
  }));

  EXPECT_SPELLING_SUGGESTION_RESULTS(archive, "patent", 10, ({
    "<b>patient</b>",
  }));

  EXPECT_SPELLING_SUGGESTION_RESULTS(archive, "lorem ipsum hebras", 10, ({
    "lorem ipsum <b>zebras</b>",
    "lorem ipsum <b>hebrew</b>",
  }));
}

std::vector<SuggestionTuple> getSmartSuggestions(const zim::Archive& archive,
                                                 std::string query,
                                                 int maxSuggestions) {
  zim::SuggestionSearcher suggestionSearcher(archive);
  suggestionSearcher.setVerbose(true);
  auto suggestionSearch = suggestionSearcher.suggest(query);
  std::vector<SuggestionTuple> result;
  for (const auto& s : suggestionSearch.getSmartSuggestions(maxSuggestions)) {
    const SuggestionTuple sTuple{s.getTitle(), s.getPath(), s.getSnippet()};
    result.push_back(sTuple);
  }
  return result;
}

#define EXPECT_SMART_SUGGESTION_RESULTS(archive, query, maxSuggestions, parenthesizedExpectedResult) \
  ASSERT_EQ(                                                                   \
      getSmartSuggestions(archive, query, maxSuggestions),                     \
      std::vector<SuggestionTuple> parenthesizedExpectedResult                 \
  )

TEST(Suggestion, smartSuggestions) {
  TempZimArchiveMadeOfEmptyHtmlArticles tza("en", {
    //{ path         , title                      }
      { "2001/01/15" , "Wikipedia Day"            },
      { "1966/08/07" , "J. Wales' birth date"     },
      { "-1/12/25"   , "Birth date of J. Christ"  },
      { "*/06/29"    , "The Little Prince Day"    },
      { "1970+/04/22", "Earth Day"                },
      { "*/11/0[12]" , "Day of the Dead"          },
      { "-14e9/11/11", "Big Bang"                 },
      { "7/2025/59"  , "invalid date"             },
      { "/etc/passwd", "User account data"        },
      { "Date_palm"  , "Date palm"                },
      { "Date_(city)", "Date, Fukushima"          },
      { "xx/xx/xx"   , "Birth date of John Smith" },
      { "^B"         , "Daily birth control"      },
      { "USbirthdata", "US birth data"            },
      { "long_ago"   , "Date of my birth"         },
  });

  zim::Archive archive(tza.getPath());

  EXPECT_SMART_SUGGESTION_RESULTS(archive, "bi", 10, ({
    {"Big Bang", "-14e9/11/11", "<b>Big</b> Bang"},
    {"Daily birth control", "^B", "Daily <b>birth</b> control"},
    {"US birth data", "USbirthdata", "US <b>birth</b> data"},
    {"Date of my birth", "long_ago", "Date of my <b>birth</b>"},
    {"J. Wales' birth date",   "1966/08/07", "J. Wales' <b>birth</b> date"},
    {"Birth date of J. Christ", "-1/12/25" , "<b>Birth</b> date of J. Christ"},
    {"Birth date of John Smith", "xx/xx/xx", "<b>Birth</b> date of John Smith"},
  }));

  // Since the count of title suggestions will exceed the specified limit,
  // autocompletion suggestions should be returned instead
  EXPECT_SMART_SUGGESTION_RESULTS(archive, "bi", 4, ({
    {"", "", "<b>big</b>" },
    {"", "", "<b>birth</b>" },
    {"", "", "<b>big</b>" }, // XXX: duplicated result due to spelling correction
  }));

  EXPECT_SMART_SUGGESTION_RESULTS(archive, "date bi", 10, ({
    {"Date of my birth", "long_ago", "<b>Date</b> of my <b>birth</b>"},
    {"J. Wales' birth date",   "1966/08/07", "J. Wales' <b>birth</b> <b>date</b>"},
    {"Birth date of J. Christ", "-1/12/25" , "<b>Birth</b> <b>date</b> of J. Christ"},
    {"Birth date of John Smith", "xx/xx/xx", "<b>Birth</b> <b>date</b> of John Smith"},
  }));

  EXPECT_SMART_SUGGESTION_RESULTS(archive, "date bi", 3, ({
    {"", "", "date <b>big</b>" },
    {"", "", "date <b>birth</b>" },
    {"", "", "date <b>big</b>" }, // XXX: duplicated result due to spelling correction
  }));

  EXPECT_SMART_SUGGESTION_RESULTS(archive, "da", 20, ({
    {"Date palm"              , "Date_palm"  , "<b>Date</b> palm"             },
    {"Date, Fukushima"        , "Date_(city)", "<b>Date</b>, Fukushima"       },
    {"Earth Day"              , "1970+/04/22", "Earth <b>Day</b>"             },
    {"Wikipedia Day"          , "2001/01/15" , "Wikipedia <b>Day</b>"         },
    {"invalid date"           , "7/2025/59"  , "invalid <b>date</b>"          },
    {"Daily birth control"    , "^B"         , "<b>Daily</b> birth control"   },
    {"US birth data"          , "USbirthdata", "US birth <b>data</b>"         },
    {"User account data"      , "/etc/passwd", "User account <b>data</b>"     },
    {"Date of my birth"       , "long_ago"   , "<b>Date</b> of my birth"      },
    {"Day of the Dead"        , "*/11/0[12]" , "<b>Day</b> of the Dead"       },
    {"J. Wales' birth date"   , "1966/08/07" , "J. Wales' birth <b>date</b>"  },
    {"The Little Prince Day"  , "*/06/29"    , "The Little Prince <b>Day</b>" },
    {"Birth date of J. Christ", "-1/12/25", "Birth <b>date</b> of J. Christ"  },
    {"Birth date of John Smith", "xx/xx/xx", "Birth <b>date</b> of John Smith"},
  }));

  // Since the count of title suggestions will exceed the specified limit,
  // autocompletion suggestions should be returned instead
  EXPECT_SMART_SUGGESTION_RESULTS(archive, "da", 5, ({
    {"", "", "<b>daily</b>" },
    {"", "", "<b>data</b>" },
    {"", "", "<b>date</b>" },
    {"", "", "<b>day</b>"  },
    {"", "", "<b>day</b>"  }, // XXX: duplicated result due to spelling correction
  }));

  // Autocompletion results are selected based on frequency ("daily" and "data"
  // are dropped as the least popular terms in the titles)
  EXPECT_SMART_SUGGESTION_RESULTS(archive, "da", 2, ({
    {"", "", "<b>date</b>" },
    {"", "", "<b>day</b>"  },
  }));

  EXPECT_SMART_SUGGESTION_RESULTS(archive, "birth da", 10, ({
    {"Daily birth control", "^B", "<b>Daily</b> <b>birth</b> control"},
    {"US birth data", "USbirthdata", "US <b>birth</b> <b>data</b>"},
    {"Date of my birth", "long_ago", "<b>Date</b> of my <b>birth</b>"},
    {"J. Wales' birth date", "1966/08/07", "J. Wales' <b>birth</b> <b>date</b>"},
    {"Birth date of J. Christ", "-1/12/25", "<b>Birth</b> <b>date</b> of J. Christ"},
    {"Birth date of John Smith", "xx/xx/xx", "<b>Birth</b> <b>date</b> of John Smith"},
  }));

  EXPECT_SMART_SUGGESTION_RESULTS(archive, "birth da", 5, ({
    {"", "", "birth <b>daily</b>" },
    {"", "", "birth <b>data</b>" },
    {"", "", "birth <b>date</b>" },
    {"", "", "birth <b>day</b>"  },
    {"", "", "birth <b>day</b>"  }, // XXX: duplicated result due to spelling correction
  }));

  EXPECT_SMART_SUGGESTION_RESULTS(archive, "barth", 5, ({
    {"", "", "<b>birth</b>" },
    {"", "", "<b>earth</b>" },
  }));
}

zim::Entry getTitleIndexEntry(const zim::Archive& a)
{
  return a.getEntryByPathWithNamespace('X', "title/xapian");
}

// To secure compatibity of new zim files with older kiwixes, we need to index
// full path of the entries as data of documents.
TEST(Suggestion, indexFullPath) {
  TempZimArchiveMadeOfEmptyHtmlArticles tza("en", {
    //{ path               , title                      }
      { "MainPage"         , "Table of Contents"        },
      { "Preface"          , "Preface"                  },
      { "Volume1/Chapter1" , "The Rise of Blefuscu"     },
      { "Volume1/Chapter2" , "Blefuscu at its Peak"     },
      { "Volume2/Chapter3" , "War with Lilliput"        },
      { "Volume2/Chapter4" , "Awakening"                },
      { "Postbutt"         , "Sadbuttrue"               },
  });

  zim::Archive archive(tza.getPath());
  const zim::Entry titleIndexEntry = getTitleIndexEntry(archive);
  const auto dai = titleIndexEntry.getItem().getDirectAccessInformation();

  ASSERT_TRUE(dai.isValid());

  Xapian::Database database;
  ASSERT_TRUE(zim::getDbFromAccessInfo(dai, database));
  const auto lastdocid = database.get_lastdocid();
  ASSERT_EQ(lastdocid, 7);

  // Make sure that the namespace is included in the path recorded
  // with each indexed document
  ASSERT_EQ(database.get_document(1).get_data(), "C/MainPage");
  ASSERT_EQ(database.get_document(2).get_data(), "C/Preface");
  ASSERT_EQ(database.get_document(3).get_data(), "C/Volume1/Chapter1");
  ASSERT_EQ(database.get_document(4).get_data(), "C/Volume1/Chapter2");
  ASSERT_EQ(database.get_document(5).get_data(), "C/Volume2/Chapter3");
  ASSERT_EQ(database.get_document(6).get_data(), "C/Volume2/Chapter4");
  ASSERT_EQ(database.get_document(7).get_data(), "C/Postbutt");
}

} // unnamed namespace
