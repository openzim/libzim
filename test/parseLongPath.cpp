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

using namespace zim;

namespace
{
TEST(ParseLongPathTest, invalid)
{
  ASSERT_THROW(parseLongPath(""), std::runtime_error);
  ASSERT_THROW(parseLongPath("A"), std::runtime_error);
  ASSERT_THROW(parseLongPath("/"), std::runtime_error);
  ASSERT_THROW(parseLongPath("//"), std::runtime_error);
  ASSERT_THROW(parseLongPath("/A"), std::runtime_error);
  ASSERT_THROW(parseLongPath("/A/"), std::runtime_error);
  ASSERT_THROW(parseLongPath("/AB"), std::runtime_error);
  ASSERT_THROW(parseLongPath("//A/path"), std::runtime_error);
}

TEST(ParseLongPathTest, valid)
{
  char ns;
  std::string path;

  std::tie(ns, path) = parseLongPath("A/path");
  ASSERT_EQ(ns, 'A');
  ASSERT_EQ(path, "path");

  std::tie(ns, path) = parseLongPath("/A/path");
  ASSERT_EQ(ns, 'A');
  ASSERT_EQ(path, "path");

  std::tie(ns, path) = parseLongPath("A//path");
  ASSERT_EQ(ns, 'A');
  ASSERT_EQ(path, "/path");

  std::tie(ns, path) = parseLongPath("/A//path");
  ASSERT_EQ(ns, 'A');
  ASSERT_EQ(path, "/path");
}
};
