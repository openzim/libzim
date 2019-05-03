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

#include "gtest/gtest.h"
#ifdef _WIN32
# include <windows.h>
# include <synchapi.h>
#else
# include <unistd.h>
#endif

namespace
{
TEST(UuidTest, construct)
{
  zim::Uuid uuid1(
      "\x01\x23\x45\x67\x89\xab\xcd\xef\x10\x32\x54\x76\x98\xba\xdc\xfe");
  zim::Uuid uuid2(
      "\x01\x23\x45\x67\x89\xab\xcd\xe0\x10\x32\x54\x76\x98\xba\xdc\x0e");

  ASSERT_TRUE(uuid1 != uuid2);
  ASSERT_TRUE(uuid1 != zim::Uuid());
  ASSERT_TRUE(uuid2 != zim::Uuid());

  ASSERT_EQ(uuid1.data[0], '\x01');
  ASSERT_EQ(uuid1.data[1], '\x23');
  ASSERT_EQ(uuid1.data[2], '\x45');
  ASSERT_EQ(uuid1.data[3], '\x67');
  ASSERT_EQ(uuid1.data[4], '\x89');
  ASSERT_EQ(uuid1.data[5], '\xab');
  ASSERT_EQ(uuid1.data[6], '\xcd');
  ASSERT_EQ(uuid1.data[7], '\xef');
  ASSERT_EQ(uuid1.data[8], '\x10');
  ASSERT_EQ(uuid1.data[9], '\x32');
  ASSERT_EQ(uuid1.data[10], '\x54');
  ASSERT_EQ(uuid1.data[11], '\x76');
  ASSERT_EQ(uuid1.data[12], '\x98');
  ASSERT_EQ(uuid1.data[13], '\xba');
  ASSERT_EQ(uuid1.data[14], '\xdc');
  ASSERT_EQ(uuid1.data[15], '\xfe');

  ASSERT_EQ(uuid2.data[0], '\x01');
  ASSERT_EQ(uuid2.data[1], '\x23');
  ASSERT_EQ(uuid2.data[2], '\x45');
  ASSERT_EQ(uuid2.data[3], '\x67');
  ASSERT_EQ(uuid2.data[4], '\x89');
  ASSERT_EQ(uuid2.data[5], '\xab');
  ASSERT_EQ(uuid2.data[6], '\xcd');
  ASSERT_EQ(uuid2.data[7], '\xe0');
  ASSERT_EQ(uuid2.data[8], '\x10');
  ASSERT_EQ(uuid2.data[9], '\x32');
  ASSERT_EQ(uuid2.data[10], '\x54');
  ASSERT_EQ(uuid2.data[11], '\x76');
  ASSERT_EQ(uuid2.data[12], '\x98');
  ASSERT_EQ(uuid2.data[13], '\xba');
  ASSERT_EQ(uuid2.data[14], '\xdc');
  ASSERT_EQ(uuid2.data[15], '\x0e');
}

TEST(UuidTest, generate)
{
  zim::Uuid uuid1;
  zim::Uuid uuid2;
  ASSERT_TRUE(uuid1 == uuid2);
  ASSERT_TRUE(uuid1 == zim::Uuid());
  ASSERT_TRUE(uuid2 == zim::Uuid());

  uuid1 = zim::Uuid::generate();
  ASSERT_TRUE(uuid1 != uuid2);
  ASSERT_TRUE(uuid1 != zim::Uuid());
  ASSERT_TRUE(uuid2 == zim::Uuid());

  // Since GNU Mach's clock isn't precise hence the time might be
  // same during generating uuid1 and uuid2 leading to test
  // failure. To bring the time difference between 2 sleep for a
  // second. Thanks to Pino Toscano.
#ifdef _WIN32
  Sleep(1000);
#else
  sleep(1);
#endif

  uuid2 = zim::Uuid::generate();
  ASSERT_TRUE(uuid1 != uuid2);
  ASSERT_TRUE(uuid1 != zim::Uuid());
  ASSERT_TRUE(uuid2 != zim::Uuid());
}

TEST(UuidTest, output)
{
  zim::Uuid uuid(
      "\x55\x0e\x84\x00\xe2\x9b\x41\xd4\xa7\x16\x44\x66\x55\x44\x00\x00");
  std::ostringstream out;
  out << uuid;
  std::string s = out.str();
  ASSERT_EQ(s, "550e8400-e29b-41d4-a716-446655440000");
}
};

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
