/*
 * Copyright (C) 2020 Veloman Yunkan
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

#include "buffer.h"
#include "bufferstreamer.h"
#include "endian_tools.h"

#include "gtest/gtest.h"

namespace
{

using namespace zim;

////////////////////////////////////////////////////////////////////////////////
// BufferStreamer
////////////////////////////////////////////////////////////////////////////////

TEST(BufferStreamer, shouldJustWork)
{
  char data[] = "abcdefghijklmnopqrstuvwxyz";
  zim::toLittleEndian(uint32_t(1234), data);
  zim::toLittleEndian(int64_t(-987654321), data+18);

  auto buffer = Buffer::makeBuffer(data, zsize_t(sizeof(data)));
  zim::BufferStreamer bds(buffer, zsize_t(sizeof(data)));

  ASSERT_EQ(1234U, bds.read<uint32_t>());

  ASSERT_EQ(data + 4, bds.current());
  const auto blob1 = std::string(bds.current(), 4);
  bds.skip(zsize_t(4));
  ASSERT_EQ("efgh", blob1);

  ASSERT_EQ(data + 8, bds.current());
  const auto blob2 = std::string(bds.current(), 10);
  bds.skip(zsize_t(10));
  ASSERT_EQ("ijklmnopqr", blob2);

  ASSERT_EQ(-987654321,   bds.read<int64_t>());
}

} // unnamed namespace
