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

#define EXPECT_SPELLING_CORRECTION(archive, query, maxSuggestions, parenthesizedExpectedResult) \
  EXPECT_EQ(                                                                   \
      getSpellingSuggestions(archive, query, maxSuggestions),                  \
      std::vector<std::string> parenthesizedExpectedResult                     \
  )

TEST(Suggestion, spellingSuggestions) {
  TempZimArchive tza("testZim");
  const zim::Archive a = tza.createZimFromTitles({
      "Abenteuer",
      "Applaus",
      "Assistent",
      "Attacke",
      "Bewandtnis",
      "Biene",
      "Botschafter",
      "Chaos",
      "Entgelt",
      "Entzündung",
      "Fahrradschloss",
      "Führerschein",
      "Gral",
      "Hierarchie",
      "Honig",
      "Impfung",
      "Kamera",
      "Konkurrenz",
      "Lachs",
      "Mond",
      "Pflaster",
      "Phänomen",
      "Prise",
      "Schirmmütze",
      "Sohn",
      "Stuhl",
      "Teller",
      "Thermoskanne",
      "Trog",
      "Umweltstandard",
      "Unfug",
      "Wohnzimmer",
      "Zunge",
      "aber",
      "abonnieren",
      "amtieren",
      "attestieren",
      "ausleeren",
      "beißen",
      "ebenfalls",
      "enttäuschen",
      "fort",
      "gefleckt",
      "gefährlich",
      "gestern",
      "gewähren",
      "hässlich",
      "konkurrieren",
      "kämmen",
      "lustig",
      "müssen",
      "nämlich",
      "runterfallen",
      "sanft",
      "schubsen",
      "seit",
      "vorgestern",
      "wahrscheinlich",

      // Entries for demonstrating shortcomings of the PoC implementation
      "Lorem ipsum",
      "King",
      "Kong",
  });

  EXPECT_SPELLING_CORRECTION(a, "", 1, ({}));

  EXPECT_SPELLING_CORRECTION(a, "geflekt", 1, ({"gefleckt"}));
  EXPECT_SPELLING_CORRECTION(a, "Teler", 1, ({"Teller"}));
  EXPECT_SPELLING_CORRECTION(a, "Teler", 1, ({"Teller"}));
  EXPECT_SPELLING_CORRECTION(a, "kämen", 1, ({"kämmen"}));
  EXPECT_SPELLING_CORRECTION(a, "abonieren", 1, ({"abonnieren"}));
  EXPECT_SPELLING_CORRECTION(a, "abbonnieren", 1, ({"abonnieren"}));
  EXPECT_SPELLING_CORRECTION(a, "abbonieren", 1, ({"abonnieren"}));
  EXPECT_SPELLING_CORRECTION(a, "Aplaus", 1, ({"Applaus"}));
  EXPECT_SPELLING_CORRECTION(a, "konkurieren", 1, ({"konkurrieren"}));
  EXPECT_SPELLING_CORRECTION(a, "Asisstent", 1, ({"Assistent"}));
  EXPECT_SPELLING_CORRECTION(a, "Assisstent", 1, ({"Assistent"}));
  EXPECT_SPELLING_CORRECTION(a, "Atacke", 1, ({"Attacke"}));
  EXPECT_SPELLING_CORRECTION(a, "atestieren", 1, ({"attestieren"}));
  EXPECT_SPELLING_CORRECTION(a, "entäuschen", 1, ({"enttäuschen"}));
  EXPECT_SPELLING_CORRECTION(a, "Enzündung", 1, ({"Entzündung"}));
  EXPECT_SPELLING_CORRECTION(a, "Schirmütze", 1, ({"Schirmmütze"}));
  EXPECT_SPELLING_CORRECTION(a, "Termoskanne", 1, ({"Thermoskanne"}));
  EXPECT_SPELLING_CORRECTION(a, "Tsunge", 1, ({"Zunge"}));
  EXPECT_SPELLING_CORRECTION(a, "vort", 1, ({"fort"}));
  EXPECT_SPELLING_CORRECTION(a, "Schtuhl", 1, ({"Stuhl"}));
  EXPECT_SPELLING_CORRECTION(a, "beissen", 1, ({"beißen"}));
  EXPECT_SPELLING_CORRECTION(a, "Camera", 1, ({"Kamera"}));
  EXPECT_SPELLING_CORRECTION(a, "Kaos", 1, ({"Chaos"}));

  // The spelling correction "Lax -> Lachs" is not returned because the max
  // edit distance is capped at (length(query_word) - 1) which reduces our
  // passed value of the max edit distance argument from 3 to 2. This
  // change was brought by
  // https://github.com/xapian/xapian/commit/0cbe35de5c392623388946e6769aa03f912fdde4
  // and first appears in v1.4.19 release of Xapian.
  //EXPECT_SPELLING_CORRECTION(a, "Lax", 1, ({"Lachs"}));
  EXPECT_SPELLING_CORRECTION(a, "Lax", 1, ({}));

  EXPECT_SPELLING_CORRECTION(a, "Mont", 1, ({"Mond"}));
  EXPECT_SPELLING_CORRECTION(a, "Umweltstandart", 1, ({"Umweltstandard"}));
  EXPECT_SPELLING_CORRECTION(a, "seid", 1, ({"seit"}));
  EXPECT_SPELLING_CORRECTION(a, "Trok", 1, ({"Trog"}));
  EXPECT_SPELLING_CORRECTION(a, "Unfuk", 1, ({"Unfug"}));
  EXPECT_SPELLING_CORRECTION(a, "schupsen", 1, ({"schubsen"}));
  EXPECT_SPELLING_CORRECTION(a, "warscheinlich", 1, ({"wahrscheinlich"}));
  EXPECT_SPELLING_CORRECTION(a, "gefärlich", 1, ({"gefährlich"}));
  EXPECT_SPELLING_CORRECTION(a, "Son", 1, ({"Sohn"}));
  EXPECT_SPELLING_CORRECTION(a, "nähmlich", 1, ({"nämlich"}));
  EXPECT_SPELLING_CORRECTION(a, "Grahl", 1, ({"Gral"}));
  EXPECT_SPELLING_CORRECTION(a, "Bine", 1, ({"Biene"}));
  EXPECT_SPELLING_CORRECTION(a, "Hirarchie", 1, ({"Hierarchie"}));
  EXPECT_SPELLING_CORRECTION(a, "Priese", 1, ({"Prise"}));
  EXPECT_SPELLING_CORRECTION(a, "auslehren", 1, ({"ausleeren"}));
  EXPECT_SPELLING_CORRECTION(a, "Phenomen", 1, ({"Phänomen"}));
  EXPECT_SPELLING_CORRECTION(a, "Phänomän", 1, ({"Phänomen"}));
  EXPECT_SPELLING_CORRECTION(a, "Phenomän", 1, ({"Phänomen"}));
  EXPECT_SPELLING_CORRECTION(a, "gewehren", 1, ({"gewähren"}));
  EXPECT_SPELLING_CORRECTION(a, "aba", 1, ({"aber"}));
  EXPECT_SPELLING_CORRECTION(a, "gestan", 1, ({"gestern"}));
  EXPECT_SPELLING_CORRECTION(a, "ronterfallen", 1, ({"runterfallen"}));
  EXPECT_SPELLING_CORRECTION(a, "Hönig", 1, ({"Honig"}));
  EXPECT_SPELLING_CORRECTION(a, "mussen", 1, ({"müssen"}));
  EXPECT_SPELLING_CORRECTION(a, "Bewandnis", 1, ({"Bewandtnis"}));
  EXPECT_SPELLING_CORRECTION(a, "hässlig", 1, ({"hässlich"}));
  EXPECT_SPELLING_CORRECTION(a, "lustich", 1, ({"lustig"}));
  EXPECT_SPELLING_CORRECTION(a, "Botschaftler", 1, ({"Botschafter"}));
  EXPECT_SPELLING_CORRECTION(a, "ebemfalls", 1, ({"ebenfalls"}));
  EXPECT_SPELLING_CORRECTION(a, "samft", 1, ({"sanft"}));
  EXPECT_SPELLING_CORRECTION(a, "Wohenzimmer", 1, ({"Wohnzimmer"}));
  EXPECT_SPELLING_CORRECTION(a, "Flaster", 1, ({"Pflaster"}));
  EXPECT_SPELLING_CORRECTION(a, "Imfung", 1, ({"Impfung"}));
  EXPECT_SPELLING_CORRECTION(a, "amptieren", 1, ({"amtieren"}));
  EXPECT_SPELLING_CORRECTION(a, "Endgeld", 1, ({"Entgelt"}));
  EXPECT_SPELLING_CORRECTION(a, "Abendteuer", 1, ({"Abenteuer"}));
  EXPECT_SPELLING_CORRECTION(a, "sampft", 1, ({"sanft"}));
  EXPECT_SPELLING_CORRECTION(a, "forgestan", 1, ({"vorgestern"}));
  EXPECT_SPELLING_CORRECTION(a, "Füreschein", 1, ({"Führerschein"}));
  EXPECT_SPELLING_CORRECTION(a, "ronterfalen", 1, ({"runterfallen"}));
  EXPECT_SPELLING_CORRECTION(a, "Farradschluss", 1, ({"Fahrradschloss"}));
  EXPECT_SPELLING_CORRECTION(a, "Konkorenz", 1, ({"Konkurrenz"}));
  EXPECT_SPELLING_CORRECTION(a, "Hirachie", 1, ({"Hierarchie"}));

  //////////////////////////////////////////////////////////////////////////////
  // Edge cases
  //////////////////////////////////////////////////////////////////////////////

  // Exact match is not considered a spelling correction
  EXPECT_SPELLING_CORRECTION(a, "Führerschein", 1, ({}));

  // Max edit distance is 3
  EXPECT_SPELLING_CORRECTION(a,   "Führersch",    1, ({"Führerschein"}));
    EXPECT_SPELLING_CORRECTION(a, "Führersc",     1, ({}));
    // Case matters in edit distance
    EXPECT_SPELLING_CORRECTION(a, "führersch",    1, ({}));
    // Diacritics matters in edit distance
    EXPECT_SPELLING_CORRECTION(a, "Fuhrersch",    1, ({}));
    // Mismatch in diacritics counts as 1 in edit distance (this is not trivial,
    // because from the UTF-8 perspective it is a one-byte vs two-byte encoding
    // of a Unicode codepoint).
    EXPECT_SPELLING_CORRECTION(a, "Führersche",   1, ({"Führerschein"}));

  EXPECT_SPELLING_CORRECTION(a, "Führershine",  1, ({"Führerschein"}));
    EXPECT_SPELLING_CORRECTION(a, "Führershyne",  1, ({}));
    EXPECT_SPELLING_CORRECTION(a, "führershine",  1, ({}));

  EXPECT_SPELLING_CORRECTION(a, "Führerschrom", 1, ({"Führerschein"}));
  EXPECT_SPELLING_CORRECTION(a, "Führerscdrom", 1, ({}));

  //////////////////////////////////////////////////////////////////////////////
  // Shortcomings of the proof-of-concept implementation
  //////////////////////////////////////////////////////////////////////////////

  // Multiword titles are treated as a single entity
  EXPECT_SPELLING_CORRECTION(a, "Laurem", 1, ({}));
  EXPECT_SPELLING_CORRECTION(a, "ibsum",  1, ({}));
  EXPECT_SPELLING_CORRECTION(a, "Loremipsum", 1, ({"Lorem ipsum"}));

  // Only one spelling correction can be requested
  // EXPECT_SPELLING_CORRECTION(a, "Kung",  2, ({"King", "Kong"}));
  EXPECT_THROW(getSpellingSuggestions(a, "Kung", 2), std::runtime_error);

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
