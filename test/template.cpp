/*
 * Copyright (C) 2009 Tommi Maekitalo
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

#include "../src/template.h"

#include "gtest/gtest.h"

namespace
{
class TemplateTest : public ::testing::Test, private zim::TemplateParser::Event
{
 public:
  std::string result;
  zim::TemplateParser parser;

  TemplateTest() : parser(this) {}

 private:
  void onData(const std::string& data) { result += data; }

  void onToken(const std::string& token)
  {
    result += "T(";
    result += token;
    result += ')';
  }

  void onLink(char ns, const std::string& title)
  {
    result += "L(";
    result += ns;
    result += ", ";
    result += title;
    result += ')';
  }
};

TEST_F(TemplateTest, ZeroTemplate)
{
  parser.parse("<html><body><h1>Hi</h1></body></html>");
  parser.flush();

  ASSERT_EQ(result, "<html><body><h1>Hi</h1></body></html>");
}

TEST_F(TemplateTest, Token)
{
  parser.parse("<html><%content%></html>");
  parser.flush();

  ASSERT_EQ(result, "<html>T(content)</html>");
}

TEST_F(TemplateTest, Link)
{
  parser.parse("<html><%/A/Article%></html>");
  parser.flush();

  ASSERT_EQ(result, "<html>L(A, Article)</html>");
}

}  // namespace

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
