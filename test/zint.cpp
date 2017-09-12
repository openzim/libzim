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

#include <sstream>
#include "../src/zintstream.h"

#include "gtest/gtest.h"

namespace
{
void testNumber(zim::size_type num)
{
  std::stringstream data;
  zim::ZIntStream zint(data);

  zint.put(num);
  unsigned n;
  zint.get(n);

  ASSERT_EQ(n, num);
  ASSERT_EQ(data.get(), std::ios::traits_type::eof());
}

TEST(ZintTest, zcompress1)
{
  testNumber(34);
}

TEST(ZintTest, zcompress2)
{
  testNumber(128);
  testNumber(234);
}

TEST(ZintTest, zcompress3)
{
  testNumber(17000);
  testNumber(16512);
}

}  // namespace

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
