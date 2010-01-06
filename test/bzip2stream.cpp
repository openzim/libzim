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

#include <zim/bunzip2stream.h>
#include <zim/bzip2stream.h>
#include <iostream>
#include <sstream>

#include <cxxtools/unit/testsuite.h>
#include <cxxtools/unit/registertest.h>

class Bzip2streamTest : public cxxtools::unit::TestSuite
{
    std::string testtext;

  public:
    Bzip2streamTest()
      : cxxtools::unit::TestSuite("zim::Bzip2streamTest")
    {
      registerMethod("bunzip2Istream", *this, &Bzip2streamTest::bunzip2IstreamTest);
      registerMethod("bunzip2Ostream", *this, &Bzip2streamTest::bunzip2OstreamTest);

      for (unsigned n = 0; n < 10240; ++n)
        testtext += "Hello";
    }

    void bunzip2IstreamTest()
    {
      // test 
      std::stringstream bzip2target;

      {
        zim::Bzip2Stream compressor(bzip2target);
        compressor << testtext << std::flush;
        compressor.end();
      }

      {
        std::ostringstream msg;
        msg << "teststring with " << testtext.size() << " bytes compressed into " << bzip2target.str().size() << " bytes";
        reportMessage(msg.str());
      }

      std::ostringstream bunzip2target;

      {
        zim::Bunzip2Stream bunzip2(bzip2target);
        bunzip2target << bunzip2.rdbuf() << std::flush; // bunzip2 is a istream here
      }

      {
        std::ostringstream msg;
        msg << "teststring uncompressed to " << bunzip2target.str().size() << " bytes";
        reportMessage(msg.str());
      }

      CXXTOOLS_UNIT_ASSERT_EQUALS(testtext, bunzip2target.str());
    }

    void bunzip2OstreamTest()
    {
      // test 
      std::stringstream bzip2target;

      {
        zim::Bzip2Stream compressor(bzip2target);
        compressor << testtext << std::flush;
        compressor.end();
      }

      {
        std::ostringstream msg;
        msg << "teststring with " << testtext.size() << " bytes compressed into " << bzip2target.str().size() << " bytes";
        reportMessage(msg.str());
      }

      std::ostringstream bunzip2target;

      {
        zim::Bunzip2Stream bunzip2(bunzip2target); // bunzip2 is a ostream here
        bunzip2 << bzip2target.str() << std::flush;
      }

      {
        std::ostringstream msg;
        msg << "teststring uncompressed to " << bunzip2target.str().size() << " bytes";
        reportMessage(msg.str());
      }

      CXXTOOLS_UNIT_ASSERT_EQUALS(testtext, bunzip2target.str());
    }

};

cxxtools::unit::RegisterTest<Bzip2streamTest> register_Bzip2streamTest;
