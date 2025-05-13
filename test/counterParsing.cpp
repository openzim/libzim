/*
 * Copyright (C) 2019 Matthieu Gautier
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
#include "../src/tools.h"

using namespace zim;
const auto parse = parseMimetypeCounter;

namespace
{
TEST(ParseCounterTest, simpleMimeType)
{
  {
    std::string counterStr = "";
    MimeCounterType counterMap = {};
    ASSERT_EQ(parse(counterStr), counterMap) << counterStr;
  }
  {
    std::string counterStr = "foo=1";
    MimeCounterType counterMap = {{"foo", 1}};
    ASSERT_EQ(parse(counterStr), counterMap) << counterStr;
  }
  {
    std::string counterStr = "foo=1;text/html=50;";
    MimeCounterType counterMap = {{"foo", 1}, {"text/html", 50}};
    ASSERT_EQ(parse(counterStr), counterMap) << counterStr;
  }
}

TEST(ParseCounterTest, paramMimeType)
{
  {
    std::string counterStr = "text/html;raw=true=1";
    MimeCounterType counterMap = {{"text/html;raw=true", 1}};
    ASSERT_EQ(parse(counterStr), counterMap) << counterStr;
  }
  {
    std::string counterStr = "foo=1;text/html;raw=true=50;bar=2";
    MimeCounterType counterMap = {{"foo", 1}, {"text/html;raw=true", 50}, {"bar", 2}};
    ASSERT_EQ(parse(counterStr), counterMap) << counterStr;
  }
  {
    std::string counterStr = "foo=1;text/html;raw=true;param=value=50;bar=2";
    MimeCounterType counterMap = {{"foo", 1}, {"text/html;raw=true;param=value", 50}, {"bar", 2}};
    ASSERT_EQ(parse(counterStr), counterMap) << counterStr;
  }
  {
    std::string counterStr = "foo=1;text/html;raw=true=50;bar=2";
    MimeCounterType counterMap = {{"foo", 1}, {"text/html;raw=true", 50}, {"bar", 2}};
    ASSERT_EQ(parse(counterStr), counterMap) << counterStr;
  }
  {
    std::string counterStr = "application/javascript=8;text/html=3;application/warc-headers=28364;text/html;raw=true=6336;text/css=47;text/javascript=98;image/png=968;image/webp=24;application/json=3694;image/gif=10274;image/jpeg=1582;font/woff2=25;text/plain=284;application/atom+xml=247;application/x-www-form-urlencoded=9;video/mp4=9;application/x-javascript=7;application/xml=1;image/svg+xml=5";
    MimeCounterType counterMap = {
      {"application/javascript", 8},
      {"text/html", 3},
      {"application/warc-headers", 28364},
      {"text/html;raw=true", 6336},
      {"text/css", 47},
      {"text/javascript", 98},
      {"image/png", 968},
      {"image/webp", 24},
      {"application/json", 3694},
      {"image/gif", 10274},
      {"image/jpeg", 1582},
      {"font/woff2", 25},
      {"text/plain", 284},
      {"application/atom+xml", 247},
      {"application/x-www-form-urlencoded", 9},
      {"video/mp4", 9},
      {"application/x-javascript", 7},
      {"application/xml", 1},
      {"image/svg+xml", 5}
    };
    ASSERT_EQ(parse(counterStr), counterMap) << counterStr;
  }
}

TEST(ParseCounterTest, wrongType)
{
  MimeCounterType empty = {};
  {
    std::string counterStr = "text/html";
    ASSERT_EQ(parse(counterStr), empty) << counterStr;
  }
  {
    std::string counterStr = "text/html=";
    ASSERT_EQ(parse(counterStr), empty) << counterStr;
  }
  {
    std::string counterStr = "text/html=foo";
    ASSERT_EQ(parse(counterStr), empty) << counterStr;
  }
  {
    std::string counterStr = "text/html=123foo";
    ASSERT_EQ(parse(counterStr), empty) << counterStr;
  }
  {
    std::string counterStr = "text/html=50;foo";
    MimeCounterType counterMap = {{"text/html", 50}};
    ASSERT_EQ(parse(counterStr), counterMap) << counterStr;
  }
  {
    std::string counterStr = "text/html;foo=20";
    ASSERT_EQ(parse(counterStr), empty) << counterStr;
  }
  {
    std::string counterStr = "text/html;foo=20;";
    ASSERT_EQ(parse(counterStr), empty) << counterStr;
  }
  {
    std::string counterStr = "text/html=50;;foo";
    MimeCounterType counterMap = {{"text/html", 50}};
    ASSERT_EQ(parse(counterStr), counterMap) << counterStr;
  }
}

#define CHECK(TEST, EXPECTED) \
{ auto count = countMimeType(counterStr, [](const std::string& s) { return TEST;}); ASSERT_EQ(count, (unsigned)EXPECTED); }

TEST(ParseCounterTest, countMimeType) {
  {
    std::string counterStr = "text/html;raw=true=1";
    CHECK(true, 1);
    CHECK(false, 0);
    CHECK(s.find("text/html") == 0, 1);
    CHECK(s.find("text/html;raw=true") == 0, 1);
  }
  {
    std::string counterStr = "foo=1;text/html;raw=true=50;bar=2";
    CHECK(true, 53);
    CHECK(false, 0);
    CHECK(s.find("text/html") == 0, 50);
    CHECK(s == "text/html", 0);
    CHECK(s.find("text/html;raw=true") == 0, 50);
    CHECK(s == "text/html;raw=true", 50);
    CHECK(s.find("text/html;raw=true;param=value") == 0, 0);
  }
  {
    std::string counterStr = "foo=1;text/html;raw=true;param=value=50;bar=2";
    CHECK(true, 53);
    CHECK(false, 0);
    CHECK(s.find("text/html") == 0, 50);
    CHECK(s.find("text/html;raw=true") == 0, 50);
    CHECK(s == "text/html;raw=true", 0);
    CHECK(s.find("text/html;raw=true;param=value") == 0, 50);
  }
  {
    std::string counterStr = "application/javascript=8;text/html=3;application/warc-headers=28364;text/html;raw=true=6336;text/css=47;text/javascript=98;image/png=968;image/webp=24;application/json=3694;image/gif=10274;image/jpeg=1582;font/woff2=25;text/plain=284;application/atom+xml=247;application/x-www-form-urlencoded=9;video/mp4=9;application/x-javascript=7;application/xml=1;image/svg+xml=5";
    CHECK(true, 51985);
    CHECK(false, 0);
    CHECK(s == "application/javascript", 8);
    CHECK(s == "text/html", 3);
    CHECK(s == "application/warc-headers", 28364);
    CHECK(s == "text/html;raw=true", 6336);
    CHECK(s == "text/css", 47);
    CHECK(s == "text/javascript", 98);
    CHECK(s == "image/png", 968);
    CHECK(s == "image/webp", 24);
    CHECK(s == "application/json", 3694);
    CHECK(s == "image/gif", 10274);
    CHECK(s == "image/jpeg", 1582);
    CHECK(s == "font/woff2", 25);
    CHECK(s == "text/plain", 284);
    CHECK(s == "application/atom+xml", 247);
    CHECK(s == "application/x-www-form-urlencoded", 9);
    CHECK(s == "video/mp4", 9);
    CHECK(s == "application/x-javascript", 7);
    CHECK(s == "application/xml", 1);
    CHECK(s == "image/svg+xml", 5);
    CHECK(s.find("text/") == 0, 3+6336+47+98+284);
    CHECK(s.find("text/html") == 0, 3+6336);
    CHECK(s.find("application/") == 0, 8+28364+3694+247+9+7+1);
    CHECK(s.find("image/") == 0, 968+24+10274+1582+5);
    CHECK(s.find("xml") != std::string::npos, 247+1+5);
    CHECK(s.find("image/") == 0 || s.find("video/") == 0 || s.find("sound/") == 0, 968+24+10274+1582+9+5);
  }
}

};
