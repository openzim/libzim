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

#include <zim/template.h>
#include <cxxtools/unit/testsuite.h>
#include <cxxtools/unit/registertest.h>

class TemplateTest : public cxxtools::unit::TestSuite, private zim::TemplateParser::Event
{
    zim::TemplateParser parser;
    std::string result;

  public:
    TemplateTest()
      : cxxtools::unit::TestSuite("zim::TemplateTest"),
        parser(this)
    {
      registerMethod("ZeroTemplate", *this, &TemplateTest::ZeroTemplate);
      registerMethod("Token", *this, &TemplateTest::Token);
      registerMethod("Link", *this, &TemplateTest::Link);
    }

    void setUp()
    {
      result.clear();
    }

    void tearDown()
    {
    }

    void ZeroTemplate()
    {
      parser.parse("<html><body><h1>Hi</h1></body></html>");
      parser.flush();

      CXXTOOLS_UNIT_ASSERT_EQUALS(result, "<html><body><h1>Hi</h1></body></html>");
    }

    void Token()
    {
      parser.parse("<html><%content%></html>");
      parser.flush();

      CXXTOOLS_UNIT_ASSERT_EQUALS(result, "<html>T(content)</html>");
    }

    void Link()
    {
      parser.parse("<html><%/A/Article%></html>");
      parser.flush();

      CXXTOOLS_UNIT_ASSERT_EQUALS(result, "<html>L(A, Article)</html>");
    }

  private:
    void onData(const std::string& data)
    {
      result += data;
    }

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

cxxtools::unit::RegisterTest<TemplateTest> register_TemplateTest;
