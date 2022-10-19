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
#define parse parseMimetypeCounter

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

