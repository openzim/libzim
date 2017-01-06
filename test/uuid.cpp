/*
 * Copyright (C) 2013 Tommi Maekitalo
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

#include <zim/uuid.h>
#include <iostream>
#include <sstream>

#include <cxxtools/unit/testsuite.h>
#include <cxxtools/unit/registertest.h>

#include <unistd.h>

class UuidTest : public cxxtools::unit::TestSuite
{
  public:
    UuidTest()
      : cxxtools::unit::TestSuite("zim::UuidTest")
    {
      registerMethod("testConstruct", *this, &UuidTest::testConstruct);
      registerMethod("testGenerate", *this, &UuidTest::testGenerate);
      registerMethod("testOutput", *this, &UuidTest::testOutput);
    }

    void testConstruct()
    {
      zim::Uuid uuid1("\x01\x23\x45\x67\x89\xab\xcd\xef\x10\x32\x54\x76\x98\xba\xdc\xfe");
      zim::Uuid uuid2("\x01\x23\x45\x67\x89\xab\xcd\xe0\x10\x32\x54\x76\x98\xba\xdc\x0e");

      CXXTOOLS_UNIT_ASSERT(uuid1 != uuid2);
      CXXTOOLS_UNIT_ASSERT(uuid1 != zim::Uuid());
      CXXTOOLS_UNIT_ASSERT(uuid2 != zim::Uuid());

      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid1.data[0], '\x01');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid1.data[1], '\x23');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid1.data[2], '\x45');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid1.data[3], '\x67');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid1.data[4], '\x89');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid1.data[5], '\xab');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid1.data[6], '\xcd');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid1.data[7], '\xef');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid1.data[8], '\x10');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid1.data[9], '\x32');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid1.data[10], '\x54');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid1.data[11], '\x76');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid1.data[12], '\x98');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid1.data[13], '\xba');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid1.data[14], '\xdc');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid1.data[15], '\xfe');

      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid2.data[0], '\x01');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid2.data[1], '\x23');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid2.data[2], '\x45');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid2.data[3], '\x67');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid2.data[4], '\x89');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid2.data[5], '\xab');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid2.data[6], '\xcd');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid2.data[7], '\xe0');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid2.data[8], '\x10');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid2.data[9], '\x32');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid2.data[10], '\x54');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid2.data[11], '\x76');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid2.data[12], '\x98');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid2.data[13], '\xba');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid2.data[14], '\xdc');
      CXXTOOLS_UNIT_ASSERT_EQUALS(uuid2.data[15], '\x0e');
    }

    void testGenerate()
    {
      zim::Uuid uuid1;
      zim::Uuid uuid2;
      CXXTOOLS_UNIT_ASSERT(uuid1 == uuid2);
      CXXTOOLS_UNIT_ASSERT(uuid1 == zim::Uuid());
      CXXTOOLS_UNIT_ASSERT(uuid2 == zim::Uuid());

      uuid1 = zim::Uuid::generate();
      CXXTOOLS_UNIT_ASSERT(uuid1 != uuid2);
      CXXTOOLS_UNIT_ASSERT(uuid1 != zim::Uuid());
      CXXTOOLS_UNIT_ASSERT(uuid2 == zim::Uuid());

      // Since GNU Mach's clock isn't precise hence the time might be
      // same during generating uuid1 and uuid2 leading to test
      // failure. To bring the time difference between 2 sleep for a
      // second. Thanks to Pino Toscano.
      sleep(1);

      uuid2 = zim::Uuid::generate();
      CXXTOOLS_UNIT_ASSERT(uuid1 != uuid2);
      CXXTOOLS_UNIT_ASSERT(uuid1 != zim::Uuid());
      CXXTOOLS_UNIT_ASSERT(uuid2 != zim::Uuid());
    }

    void testOutput()
    {
      zim::Uuid uuid("\x55\x0e\x84\x00\xe2\x9b\x41\xd4\xa7\x16\x44\x66\x55\x44\x00\x00");
      std::ostringstream out;
      out << uuid;
      std::string s = out.str();
      CXXTOOLS_UNIT_ASSERT_EQUALS(s, "550e8400-e29b-41d4-a716-446655440000");
    }

};

cxxtools::unit::RegisterTest<UuidTest> register_UuidTest;
