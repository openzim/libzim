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

#include <cxxtools/unit/testsuite.h>
#include <cxxtools/unit/registertest.h>
#include "zim/zintstream.h"
#include <sstream>

class ZIntTest : public cxxtools::unit::TestSuite
{
    void testNumber(zim::size_type num)
    {
      std::stringstream data;
      zim::ZIntStream zint(data);

      zint.put(num);
      unsigned n;
      zint.get(n);

      CXXTOOLS_UNIT_ASSERT_EQUALS(n, num);
      CXXTOOLS_UNIT_ASSERT_EQUALS(data.get(), std::ios::traits_type::eof());
    }

  public:
    ZIntTest()
      : cxxtools::unit::TestSuite("zim::ZIntTest")
    {
      registerMethod("zcompress1", *this, &ZIntTest::zcompress1);
      registerMethod("zcompress2", *this, &ZIntTest::zcompress2);
      registerMethod("zcompress3", *this, &ZIntTest::zcompress3);
    }

    void zcompress1()
    {
      testNumber(34);
    }

    void zcompress2()
    {
      testNumber(128);
      testNumber(234);
    }

    void zcompress3()
    {
      testNumber(17000);
      testNumber(16512);
    }

};

cxxtools::unit::RegisterTest<ZIntTest> register_ZIntTest;
