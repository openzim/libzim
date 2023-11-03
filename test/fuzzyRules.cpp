/*
 * Copyright (C) 2023 Matthieu Gautier
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

#include "gtest/gtest.h"
#include <string>
#include <vector>
#include <map>
#include <zim/zim.h>
#include "../src/fuzzy_rules.h"

using namespace zim;

namespace
{
TEST(ParseFuzzyRule, simpleFuzzyRule)
{
  {
    std::string fuzzyRulesDef = "";
    std::vector<FuzzyRule> rules = {};
    ASSERT_EQ(FuzzyRules(fuzzyRulesDef).rules, rules) << fuzzyRulesDef;
  }
  {
    std::string fuzzyRulesDef = "MATCH foo";
    const FuzzyRule f("foo","", "?", false, {});
    std::vector<FuzzyRule> rules = { f };
    ASSERT_EQ(FuzzyRules(fuzzyRulesDef).rules, rules) << fuzzyRulesDef;
  }
  {
    std::string fuzzyRulesDef = "MATCH foo?query\nREPLACE bar\nSPLIT ?query\nARGS baz&buz\nARGS buz";
    std::vector<FuzzyRule> rules = {
      FuzzyRule("foo?query","bar", "?query", false, {{"baz", "buz"}, {"buz"}})
    };
    ASSERT_EQ(FuzzyRules(fuzzyRulesDef).rules, rules) << fuzzyRulesDef;
  }
}

TEST(ParseFuzzyRule, severalFuzzyRule)
{
  {
    std::string fuzzyRulesDef = "MATCH foo\nMATCH bar";
    std::vector<FuzzyRule> rules = {
      FuzzyRule("foo", "", "?", false, {}),
      FuzzyRule("bar", "", "?", false, {}),
    };
    ASSERT_EQ(FuzzyRules(fuzzyRulesDef).rules, rules) << fuzzyRulesDef;
  }
  {
    std::string fuzzyRulesDef = "MATCH foo?query\nREPLACE bar\nSPLIT ?query\nARGS baz&buz\nARGS buz\nMATCH bar\nRSPLIT r";
    std::vector<FuzzyRule> rules = {
      FuzzyRule("foo?query","bar", "?query", false, {{"baz", "buz"}, {"buz"}}),
      FuzzyRule("bar", "", "r", true, {})
    };
    ASSERT_EQ(FuzzyRules(fuzzyRulesDef).rules, rules) << fuzzyRulesDef;
  }
}

TEST(ParseFuzzyRule, complexFuzzyRule)
{
  {
    // Last command is taken into acount
    std::string fuzzyRulesDef = "MATCH foo\nSPLIT o\nRSPLIT a\nREPLACE baz\nREPLACE bar";
    std::vector<FuzzyRule> rules = {
      FuzzyRule("foo", "bar", "a", true, {}),
    };
    ASSERT_EQ(FuzzyRules(fuzzyRulesDef).rules, rules) << fuzzyRulesDef;
  }
  {
    std::string fuzzyRulesDef = "MATCH foo?query\nREPLACE bar\nSPLIT ?query bar\nUnknown sentence\nARGS baz&buz\nARGS buz\n\nMATCH bar\nRSPLITr\nRSPLIT r\nnospace";
    std::vector<FuzzyRule> rules = {
      FuzzyRule("foo?query","bar", "?query bar", false, {{"baz", "buz"}, {"buz"}}),
      FuzzyRule("bar", "", "r", true, {})
    };
    ASSERT_EQ(FuzzyRules(fuzzyRulesDef).rules, rules) << fuzzyRulesDef;
  }
}

TEST(WritingFuzzyRule, writeFuzzyRules)
{
  std::vector<FuzzyRule> rules = {
    FuzzyRule("foo?query","bar", "?query", false, {{"baz", "buz"}, {"buz"}}),
    FuzzyRule("bar", "", "r", true, {})
  };
  std::ostringstream oss;
  for(const auto& rule: rules) {
    rule.write(oss);
  }
  ASSERT_EQ(oss.str(), "MATCH foo?query\nREPLACE bar\nSPLIT ?query\nARGS baz&buz\nARGS buz\nMATCH bar\nREPLACE \nRSPLIT r\n");
  ASSERT_EQ(FuzzyRules(oss.str()).rules, rules);
}

TEST(WritingFuzzyRule, realCase) {
  std::vector<FuzzyRule> rules = {
    FuzzyRule("^(https?://(?:www\\.)?)(youtube\\.com/@[^?]+)[?].*", "$1$2", "?", false, {}),
    FuzzyRule("(?:www\\.)?youtube(?:-nocookie)?\\.com/(get_video_info)", "youtube.fuzzy.replayweb.page/$1", "?", true, {{"video_id"}}),
    FuzzyRule("(?:www\\.)?youtube(?:-nocookie)?\\.com/(youtubei/v1/[^?]+\\?).*(videoId[^&]+).*", "youtube.fuzzy.replayweb.page/$1$2", "?", false, {{"videoId"}}),
    FuzzyRule(".*googlevideo.com/(videoplayback)", "youtube.fuzzy.replayweb.page/$1", "?", false, {{"id", "itags"}, {"id"}})
  };
  std::ostringstream oss;
  for(const auto& rule: rules) {
    rule.write(oss);
  }
  ASSERT_EQ(oss.str(), "MATCH ^(https?://(?:www\\.)?)(youtube\\.com/@[^?]+)[?].*\nREPLACE $1$2\nSPLIT ?\n"
                       "MATCH (?:www\\.)?youtube(?:-nocookie)?\\.com/(get_video_info)\nREPLACE youtube.fuzzy.replayweb.page/$1\nRSPLIT ?\nARGS video_id\n"
                       "MATCH (?:www\\.)?youtube(?:-nocookie)?\\.com/(youtubei/v1/[^?]+\\?).*(videoId[^&]+).*\nREPLACE youtube.fuzzy.replayweb.page/$1$2\nSPLIT ?\nARGS videoId\n"
                       "MATCH .*googlevideo.com/(videoplayback)\nREPLACE youtube.fuzzy.replayweb.page/$1\nSPLIT ?\nARGS id&itags\nARGS id\n"
            );
  ASSERT_EQ(FuzzyRules(oss.str()).rules, rules);
}


};
