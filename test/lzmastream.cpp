/*
 * Copyright (C) 2010 Tommi Maekitalo
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

#include <zim/lzmastream.h>
#include <zim/unlzmastream.h>
#include <iostream>
#include <sstream>

#include <cxxtools/unit/testsuite.h>
#include <cxxtools/unit/registertest.h>

class LzmastreamTest : public cxxtools::unit::TestSuite
{
    std::string testtext;

  public:
    LzmastreamTest()
      : cxxtools::unit::TestSuite("zim::LzmastreamTest")
    {
      registerMethod("lzmaIstream", *this, &LzmastreamTest::lzmaIstreamTest);
      registerMethod("lzmaOstream", *this, &LzmastreamTest::lzmaOstreamTest);

      for (unsigned n = 0; n < 10240; ++n)
        testtext += "Hello";
    }

    void lzmaIstreamTest()
    {
      // test 
      std::stringstream lzmatarget;
      zim::LzmaStream compressor(lzmatarget);
      compressor << testtext << std::flush;

      {
        std::ostringstream msg;
        msg << "teststring with " << testtext.size() << " bytes compressed into " << lzmatarget.str().size() << " bytes";
        reportMessage(msg.str());
      }

      zim::UnlzmaStream lzma(lzmatarget);
      std::ostringstream unlzmatarget;
      unlzmatarget << lzma.rdbuf(); // lzma is a istream here

      {
        std::ostringstream msg;
        msg << "teststring uncompressed to " << unlzmatarget.str().size() << " bytes";
        reportMessage(msg.str());
      }

      CXXTOOLS_UNIT_ASSERT_EQUALS(testtext, unlzmatarget.str());
    }

    void lzmaOstreamTest()
    {
      // test 
      std::stringstream lzmatarget;
      zim::LzmaStream compressor(lzmatarget);
      compressor << testtext << std::flush;

      {
        std::ostringstream msg;
        msg << "teststring with " << testtext.size() << " bytes compressed into " << lzmatarget.str().size() << " bytes";
        reportMessage(msg.str());
      }

      std::ostringstream unlzmatarget;
      zim::UnlzmaStream lzma(unlzmatarget); // lzma is a ostream here
      lzma << lzmatarget.str() << std::flush;

      {
        std::ostringstream msg;
        msg << "teststring uncompressed to " << unlzmatarget.str().size() << " bytes";
        reportMessage(msg.str());
      }

      CXXTOOLS_UNIT_ASSERT_EQUALS(testtext, unlzmatarget.str());
    }

};

cxxtools::unit::RegisterTest<LzmastreamTest> register_LzmastreamTest;
