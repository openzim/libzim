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

#include "istreamreader.h"
#include "bufdatastream.h"
#include "endian_tools.h"

#include "gtest/gtest.h"

namespace
{

using zim::IStreamReader;

////////////////////////////////////////////////////////////////////////////////
// IDataStream
////////////////////////////////////////////////////////////////////////////////

// Implement the IStreamReader interface in the simplest way
class InfiniteZeroStream : public IStreamReader
{
  void readImpl(char* buf, zim::zsize_t nbytes) { memset(buf, 0, nbytes.v); }
};

// ... and test that it compiles and works as intended

TEST(IStreamReader, read)
{
  InfiniteZeroStream izs;
  IStreamReader& ids = izs;
  EXPECT_EQ(0, ids.read<int>());
  EXPECT_EQ(0L, ids.read<long>());

  // zim::fromLittleEndian() handles only integer types
  // EXPECT_EQ(0.0, ids.read<double>());
}

TEST(IStreamReader, sub_reader)
{
  const size_t N = 16;
  const char zerobuf[N] = {0};
  InfiniteZeroStream izs;
  IStreamReader& ids = izs;
  auto subReader = ids.sub_reader(zim::zsize_t(N));
  EXPECT_EQ(subReader->size().v, N);
  auto buffer = subReader->get_buffer(zim::offset_t(0), zim::zsize_t(N));
  EXPECT_EQ(buffer.size().v, N);
  EXPECT_EQ(0, memcmp(buffer.data(), zerobuf, N));
}

////////////////////////////////////////////////////////////////////////////////
// BufDataStream
////////////////////////////////////////////////////////////////////////////////

std::string toString(const IDataStream::Blob& blob)
{
  return std::string(blob.data(), blob.size());
}

TEST(BufDataStream, shouldJustWork)
{
  char data[] = "abcdefghijklmnopqrstuvwxyz";
  zim::toLittleEndian(uint32_t(1234), data);
  zim::toLittleEndian(int64_t(-987654321), data+18);

  zim::BufDataStream bds(data, sizeof(data));
  IDataStream& ids = bds;

  ASSERT_EQ(1234,         ids.read<uint32_t>());

  const auto blob1 = ids.readBlob(4);
  ASSERT_EQ("efgh", toString(blob1));
  ASSERT_EQ(data + 4, blob1.data());

  const auto blob2 = ids.readBlob(10);
  ASSERT_EQ("ijklmnopqr", toString(blob2));
  ASSERT_EQ(data + 8, blob2.data());

  ASSERT_EQ(-987654321,   ids.read<int64_t>());
}

} // unnamed namespace
