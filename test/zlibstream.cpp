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

#include <zim/inflatestream.h>
#include <zim/deflatestream.h>
#include <iostream>
#include <sstream>

#include <cxxtools/unit/testsuite.h>
#include <cxxtools/unit/registertest.h>

class ZlibstreamTest : public cxxtools::unit::TestSuite
{
    std::string testtext;

  public:
    ZlibstreamTest()
      : cxxtools::unit::TestSuite("zim::ZlibstreamTest")
    {
      registerMethod("inflatorIstream", *this, &ZlibstreamTest::inflatorIstreamTest);
      registerMethod("inflatorOstream", *this, &ZlibstreamTest::inflatorOstreamTest);

      for (unsigned n = 0; n < 10240; ++n)
        testtext += "Hello";
    }

    void inflatorIstreamTest()
    {
      // test 
      std::stringstream deflatetarget;
      zim::DeflateStream deflator(deflatetarget);
      deflator << testtext << std::flush;

      {
        std::ostringstream msg;
        msg << "teststring with " << testtext.size() << " bytes compressed into " << deflatetarget.str().size() << " bytes";
        reportMessage(msg.str());
      }

      zim::InflateStream inflator(deflatetarget);
      std::ostringstream inflatetarget;
      inflatetarget << inflator.rdbuf(); // inflator is a istream here

      {
        std::ostringstream msg;
        msg << "teststring uncompressed to " << inflatetarget.str().size() << " bytes";
        reportMessage(msg.str());
      }

      CXXTOOLS_UNIT_ASSERT_EQUALS(testtext, inflatetarget.str());
    }

    void inflatorOstreamTest()
    {
      // test 
      std::stringstream deflatetarget;
      zim::DeflateStream deflator(deflatetarget);
      deflator << testtext << std::flush;

      {
        std::ostringstream msg;
        msg << "teststring with " << testtext.size() << " bytes compressed into " << deflatetarget.str().size() << " bytes";
        reportMessage(msg.str());
      }

      std::ostringstream inflatetarget;
      zim::InflateStream inflator(inflatetarget); // inflator is a ostream here
      inflator << deflatetarget.str() << std::flush;

      {
        std::ostringstream msg;
        msg << "teststring uncompressed to " << inflatetarget.str().size() << " bytes";
        reportMessage(msg.str());
      }

      CXXTOOLS_UNIT_ASSERT_EQUALS(testtext, inflatetarget.str());
    }

};

cxxtools::unit::RegisterTest<ZlibstreamTest> register_ZlibstreamTest;
