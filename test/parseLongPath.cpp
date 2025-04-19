/*
 * Copyright (C) 2020 Matthieu Gautier mgautier@kymeria.fr
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
#include <tuple>

namespace zim {
  std::tuple<char, std::string> parseLongPath(const std::string& longPath);
};

using zim::parseLongPath;

namespace
{
TEST(ParseLongPathTest, invalid)
{
  ASSERT_THROW(parseLongPath(""), std::runtime_error);
  ASSERT_THROW(parseLongPath("AB"), std::runtime_error);
  ASSERT_THROW(parseLongPath("AB/path"), std::runtime_error);
  ASSERT_THROW(parseLongPath("/"), std::runtime_error);
  ASSERT_THROW(parseLongPath("//"), std::runtime_error);
  ASSERT_THROW(parseLongPath("/AB"), std::runtime_error);
  ASSERT_THROW(parseLongPath("AB/"), std::runtime_error);
  ASSERT_THROW(parseLongPath("/AB/path"), std::runtime_error);
  ASSERT_THROW(parseLongPath("//A/path"), std::runtime_error);
}

TEST(ParseLongPathTest, valid)
{
  ASSERT_EQ(parseLongPath("A/path"), std::make_tuple('A', "path"));
  ASSERT_EQ(parseLongPath("A/p"), std::make_tuple('A', "p"));
  ASSERT_EQ(parseLongPath("/B/path"), std::make_tuple('B', "path"));
  ASSERT_EQ(parseLongPath("/B/p"), std::make_tuple('B', "p"));
  ASSERT_EQ(parseLongPath("C//path"), std::make_tuple('C', "/path"));
  ASSERT_EQ(parseLongPath("/C//path"), std::make_tuple('C', "/path"));
  ASSERT_EQ(parseLongPath("L/path/with/separator"), std::make_tuple('L', "path/with/separator"));
  ASSERT_EQ(parseLongPath("L//path/with/separator"), std::make_tuple('L', "/path/with/separator"));
  ASSERT_EQ(parseLongPath("A"), std::make_tuple('A', ""));
  ASSERT_EQ(parseLongPath("/A"), std::make_tuple('A', ""));
  ASSERT_EQ(parseLongPath("A/"), std::make_tuple('A', ""));
  ASSERT_EQ(parseLongPath("/A/"), std::make_tuple('A', ""));
}
};
