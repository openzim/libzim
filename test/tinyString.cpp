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

#include "gtest/gtest.h"

#include "../src/writer/tinyString.h"

using namespace zim::writer;

namespace
{

TEST(TinyStringTest, empty)
{
  TinyString s;
  ASSERT_TRUE(s.empty());
  ASSERT_EQ(s.size(), 0U);
  ASSERT_EQ((std::string)s, "");
  ASSERT_EQ(s, TinyString());
}

TEST(TinyStringTest, noChar)
{
  TinyString s("");
  ASSERT_TRUE(s.empty());
  ASSERT_EQ(s.size(), 0U);
  ASSERT_EQ((std::string)s, "");
  ASSERT_EQ(s, TinyString());
}

TEST(TinyStringTest, oneChar)
{
  TinyString s("A");
  ASSERT_FALSE(s.empty());
  ASSERT_EQ(s.size(), 1U);
  ASSERT_EQ((std::string)s, "A");
  ASSERT_TRUE(s < TinyString("B"));
  ASSERT_EQ(s, TinyString("A"));
  ASSERT_FALSE(s == TinyString("B"));
}

TEST(TinyStringTest, chars)
{
  TinyString s("ABCDE");
  ASSERT_FALSE(s.empty());
  ASSERT_EQ(s.size(), 5U);
  ASSERT_EQ((std::string)s, "ABCDE");
  ASSERT_FALSE(s < TinyString());
  ASSERT_FALSE(s < TinyString(""));
  ASSERT_FALSE(s < TinyString("A"));
  ASSERT_FALSE(s < TinyString("ABCD"));
  ASSERT_FALSE(s < TinyString("AACDE"));
  ASSERT_TRUE(TinyString() < s);
  ASSERT_TRUE(TinyString("") < s);
  ASSERT_TRUE(TinyString("A") < s);
  ASSERT_TRUE(TinyString("ABCD") < s);
  ASSERT_TRUE(TinyString("AACDE") < s);
  ASSERT_TRUE(s == s);
  ASSERT_FALSE(s < s);
}

TEST(PathTitleTinyString, none)
{
  PathTitleTinyString s;
  ASSERT_TRUE(s.empty());
  ASSERT_EQ(s.size(), 0U);
  ASSERT_EQ((std::string)s, "");
  ASSERT_EQ(s, TinyString());
  ASSERT_EQ(s.getPath(), "");
  ASSERT_EQ(s.getTitle(false), "");
  ASSERT_EQ(s.getTitle(true), "");
}

TEST(PathTitleTinyString, empty)
{
  //We have the separator between path and title
  PathTitleTinyString s("", "");
  ASSERT_FALSE(s.empty());
  ASSERT_EQ(s.size(), 1U);
  ASSERT_EQ((std::string)s, std::string("", 1));
  ASSERT_EQ(s.getPath(), "");
  ASSERT_EQ(s.getTitle(false), "");
  ASSERT_EQ(s.getTitle(true), "");
}

TEST(PathTitleTinyString, no_title)
{
  //We have the separator between path and title
  PathTitleTinyString s("FOO", "");
  ASSERT_FALSE(s.empty());
  ASSERT_EQ(s.size(), 4U);
  ASSERT_EQ((std::string)s, std::string("FOO\0", 4));
  ASSERT_EQ(s.getPath(), "FOO");
  ASSERT_EQ(s.getTitle(false), "FOO");
  ASSERT_EQ(s.getTitle(true), "");
}

TEST(PathTitleTinyString, no_path)
{
  //We have the separator between path and title
  PathTitleTinyString s("", "BAR");
  ASSERT_FALSE(s.empty());
  ASSERT_EQ(s.size(), 4U);
  ASSERT_EQ((std::string)s, std::string("\0BAR", 4));
  ASSERT_EQ(s.getPath(), "");
  ASSERT_EQ(s.getTitle(false), "BAR");
  ASSERT_EQ(s.getTitle(true), "BAR");
}

TEST(PathTitleTinyString, path_title)
{
  //We have the separator between path and title
  PathTitleTinyString s("FOO", "BAR");
  ASSERT_FALSE(s.empty());
  ASSERT_EQ(s.size(), 7U);
  ASSERT_EQ((std::string)s, std::string("FOO\0BAR", 7));
  ASSERT_EQ(s.getPath(), "FOO");
  ASSERT_EQ(s.getTitle(false), "BAR");
  ASSERT_EQ(s.getTitle(true), "BAR");
}

TEST(PathTitleTinyString, equal_path_title)
{
  //We have the separator between path and title
  PathTitleTinyString s("FOO", "FOO");
  ASSERT_FALSE(s.empty());
  ASSERT_EQ(s.size(), 4U);
  ASSERT_EQ((std::string)s, std::string("FOO\0", 4));
  ASSERT_EQ(s.getPath(), "FOO");
  ASSERT_EQ(s.getTitle(false), "FOO");
  ASSERT_EQ(s.getTitle(true), "");
}
}  // namespace
