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

#include "../src/tools.h"

#include "gtest/gtest.h"
#include <sstream>
#include <string>

#if defined(ENABLE_XAPIAN)
  #include <unicode/unistr.h>
#endif

namespace {

TEST(Tools, wordCount) {
  ASSERT_EQ(zim::countWords(""), 0U);
  ASSERT_EQ(zim::countWords("   "), 0U);
  ASSERT_EQ(zim::countWords("One"), 1U);
  ASSERT_EQ(zim::countWords("One Two Three"), 3U);
  ASSERT_EQ(zim::countWords("  One  "), 1U);
  ASSERT_EQ(zim::countWords("One    Two Three   "), 3U);
  ASSERT_EQ(zim::countWords("One.Two\tThree"), 2U);
}


TEST(Tools, parseIllustrationPathToSize) {
  ASSERT_EQ(zim::parseIllustrationPathToSize("Illustration_0x0@1"), 0U);
  ASSERT_EQ(zim::parseIllustrationPathToSize("Illustration_1x1@1"), 1U);
  ASSERT_EQ(zim::parseIllustrationPathToSize("Illustration_01x01@1"), 1U);
  ASSERT_EQ(zim::parseIllustrationPathToSize("Illustration_64x64@1"), 64U);
  ASSERT_EQ(zim::parseIllustrationPathToSize("Illustration_128x128@1"), 128U);
  ASSERT_EQ(zim::parseIllustrationPathToSize("Illustration_1024x1024@1"), 1024U);
  ASSERT_THROW(zim::parseIllustrationPathToSize("Illsration_64x64@1"), std::runtime_error);
  ASSERT_THROW(zim::parseIllustrationPathToSize("Illstration_"), std::runtime_error);
  ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_64x@1"), std::runtime_error);
  ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_64x"), std::runtime_error);
  ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_64x64"), std::runtime_error);
  ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_64x64@1.5"), std::runtime_error);
  ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_128x64@1"), std::runtime_error);
  ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_-32x-32@1"), std::runtime_error);
  ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_ 64x64@1"), std::runtime_error);
  ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_64x 64@1"), std::runtime_error);
  ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_ 64x 64@1"), std::runtime_error);
  ASSERT_THROW(zim::parseIllustrationPathToSize("Illustration_1 28x1 28@1"), std::runtime_error);
}

TEST(Tools, stripMimeParameters) {
  // Basic MIME types without parameters - should be unchanged
  ASSERT_EQ(zim::stripMimeParameters("text/html"), "text/html");
  ASSERT_EQ(zim::stripMimeParameters("application/json"), "application/json");
  ASSERT_EQ(zim::stripMimeParameters("image/png"), "image/png");

  // Empty string
  ASSERT_EQ(zim::stripMimeParameters(""), "");

  // MIME types with simple parameters - should strip parameter
  ASSERT_EQ(zim::stripMimeParameters("text/html;charset=utf-8"), "text/html");
  ASSERT_EQ(zim::stripMimeParameters("text/plain;charset=us-ascii"), "text/plain");

  // MIME types with space before semicolon - should trim trailing whitespace
  ASSERT_EQ(zim::stripMimeParameters("text/html ;charset=utf-8"), "text/html");
  ASSERT_EQ(zim::stripMimeParameters("text/html  ;charset=utf-8"), "text/html");
  ASSERT_EQ(zim::stripMimeParameters("text/html\t;charset=utf-8"), "text/html");

  // Multiple parameters
  ASSERT_EQ(zim::stripMimeParameters("text/html;charset=utf-8;boundary=something"), "text/html");

  // Edge case: only whitespace before semicolon
  ASSERT_EQ(zim::stripMimeParameters(" ;charset=utf-8"), "");
  ASSERT_EQ(zim::stripMimeParameters("  ;charset=utf-8"), "");
  ASSERT_EQ(zim::stripMimeParameters("\t;charset=utf-8"), "");

  // Edge case: semicolon at start
  ASSERT_EQ(zim::stripMimeParameters(";charset=utf-8"), "");

  // Edge case: just a semicolon
  ASSERT_EQ(zim::stripMimeParameters(";"), "");
}

#if defined(ENABLE_XAPIAN)
TEST(Tools, removeAccents) {
  ASSERT_EQ(zim::removeAccents("bépoàǹ"), "bepoan");
  std::ostringstream ss;
  // Create 2 and half batches (3 boundaries) of 4kb.
  // Each bondary has its property:
  // - A 4 bytes chars being cut by the boundary
  // - A "é" stored in NDF form when the "e" is before the boundary and the "´" is after
  // - Nothing special
  for(auto j=0; j<1023; j++) {
    ss << "bépo";
  }
  ss << "bép𩸽";
  for(auto j=0; j<1023; j++) {
    ss << "bépo";
  }
  ss << "bép" << "e" << "\xcc\x81";
  for (auto j=0; j<512; j++) {
    ss << "bépo";
  }
  auto accentedString(ss.str());
  // Check our input data (that we have a char in the middle of a batch boundary)
  // Indexing is made on u16
  // `zim::removeAccents` calls `ucnv_setDefaultName` before creating the UnicodeString
  // so it will be converted using the right encoding ("utf8").
  // But we don't so we need to be explicit on the encoding here.
  icu::UnicodeString ustring(accentedString.c_str(), "utf8");

  // Test input data.
  // "bépo" is 4 chars
  ASSERT_EQ(ustring.getChar32Limit(0), 0);
  ASSERT_EQ(ustring.getChar32Limit(1), 1);
  ASSERT_EQ(ustring.getChar32Limit(2), 2);
  ASSERT_EQ(ustring.getChar32Limit(3), 3);
  ASSERT_EQ(ustring.getChar32Limit(4), 4);
  // 𩸽 is in the middle of a boundary
  ASSERT_EQ(ustring.getChar32Limit(4*1024-1), 4*1024-1);
  ASSERT_EQ(ustring.getChar32Limit(4*1024), 4*1024+1);
  ASSERT_EQ(ustring.getChar32Limit(4*1024+1), 4*1024+1);
  // Because of 𩸽 at first boundary, second boundary will search at (4*1024+1) + 4*1024 so 8*1024+1
  ASSERT_EQ(ustring.getChar32Limit(8*1024), 8*1024);
  ASSERT_EQ(ustring.getChar32Limit(8*1024+1), 8*1024+1);
  ASSERT_EQ(ustring.getChar32Limit(8*1024+2), 8*1024+2);
  // boundary is in the middle of "e´"
  EXPECT_EQ(ustring[8*1024-1], 'p'); // boundary - 2
  EXPECT_EQ(ustring[8*1024], 'e'); // boundary - 1
  EXPECT_EQ(ustring[8*1024+1], 0x0301); // boundary. Unicode symbol for '´' (utf16: 0x0301, utf8 : 0xCC 0x81)
  EXPECT_EQ(ustring[8*1024+2], 'b'); // boundary + 1
  EXPECT_EQ(ustring[8*1024+3], 233 /*ascii code for é*/); // boundary + 2
  ss.str("");
  for(auto j=0; j<1023; j++) {
    ss << "bepo";
  }
  ss << "bep𩸽";
  for(auto j=0; j<1023; j++) {
    ss << "bepo";
  }
  ss << "bep" << "e";
  for (auto j=0; j<512; j++) {
    ss << "bepo";
  }
  auto unaccentedString(ss.str());
  ASSERT_EQ(zim::removeAccents(accentedString), unaccentedString);
}

#endif

TEST(UrlUtils, getFragmentComponent) {
  const std::vector<std::pair<std::string, std::string>> testData{
  //{ url                                          , fragment }
    { ""                                           , ""                },
    { "#fragment"                                  , "#fragment"       },
    { "road/to/hell"                               , ""                },
    { "path/to/prosperity#luck"                    , "#luck"           },
    { "/a/b/c#"                                    , "#"               },

    // special symbols appearing in fragment
    { "../faq#qwerty?a=1#xyz"                      , "#qwerty?a=1#xyz" },

    // search and fragment used in the same url
    { "https://agi.ai/?q=purpose+of+life#tldr"     , "#tldr"           },

    // URI-encoded #
    { "Java-vs-C%23?page=5"                        , ""                },
    { "C%23-vs-Java#cons"                          , "#cons"           },
  };

  for ( const auto& t : testData ) {
    const std::string url = t.first;
    const std::string expectedResult = t.second;
    EXPECT_EQ(zim::UrlUtils::getFragmentComponent(url), expectedResult)
      << "URL: " << url;
  }
}

TEST(UrlUtils, getSearchComponent) {
  const std::vector<std::pair<std::string, std::string>> testData{
  //{ url                                          , search component }
    { ""                                           , ""                       },
    { "?planet=new+earth"                          , "?planet=new+earth"      },
    { "landing/page/en"                            , ""                       },
    { "/favicon?size=32x32&colour=red"             , "?size=32x32&colour=red" },
    { "how-old-is-your-browser?"                   , "?"                      },

    // search and fragment used in the same url
    { "https://agi.ai/?q=purpose+of+life#tldr"     , "?q=purpose+of+life"     },

    // search followed by fragment containing a ? symbol
    { "toc?lang=sumerian#ok?"                      , "?lang=sumerian"         },

    // URI-encoded ?
    { "wiki/What%3F_Where%3F_When%3F"              , ""                       },
    { "program/What%3F_Where%3F_When%3F?y=1995"    , "?y=1995"                },
  };

  for ( const auto& t : testData ) {
    const std::string url = t.first;
    const std::string expectedResult = t.second;
    EXPECT_EQ(zim::UrlUtils::getSearchComponent(url), expectedResult)
      << "URL: " << url;
  }
}

TEST(UrlUtils, stripSearchAndFragmentComponents) {
  const std::vector<std::pair<std::string, std::string>> testData{
  //{ url                                      , search component }
    { ""                                       , ""                          },
    { "?query=who+framed+rabbit+roger"         , ""                          },
    { "#anchor"                                , ""                          },
    { "admin/login"                            , "admin/login"               },
    { "news/feed?format=rss"                   , "news/feed"                 },
    { "/pull/123/diff#file=main.js&line=123"   , "/pull/123/diff"            },
    { "/books/?author=jared+diamond#upheaval"  , "/books/"                   },

    // URI-encoded ? and # symbols
    { "Who_needs_F%23_and_why%3F"              , "Who_needs_F%23_and_why%3F" },
  };

  for ( const auto& t : testData ) {
    const std::string url = t.first;
    const std::string expectedResult = t.second;
    EXPECT_EQ(zim::UrlUtils::stripSearchAndFragmentComponents(url), expectedResult)
      << "URL: " << url;
  }
}

TEST(UrlUtils, getVirtualRedirectUrl) {

  EXPECT_EQ("",
            zim::UrlUtils::getVirtualRedirectUrl("")
  );

  EXPECT_EQ("./Nairobi#Geography",
            zim::UrlUtils::getVirtualRedirectUrl(R"(<html>
  <head>
    <title>Geography of Nairobi</title>
    <meta http-equiv="refresh" content="0;URL='./Nairobi#Geography'" />
  </head>
  <body></body>
</html>)")
  );
}


} // unnamed namespace
