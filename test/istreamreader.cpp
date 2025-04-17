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
#include "endian_tools.h"

#include "gtest/gtest.h"

namespace
{

using namespace zim;

////////////////////////////////////////////////////////////////////////////////
// IDataStream
////////////////////////////////////////////////////////////////////////////////

// Implement the IStreamReader interface in the simplest way
class InfiniteZeroStream : public IStreamReader
{
  void readImpl(char* buf, zim::zsize_t nbytes) override { memset(buf, 0, nbytes.v); }
  size_t getMemorySize() const override {
    return 0;
  }
};

class InfiniteIncreasingStream: public IStreamReader
{
  zim::offset_type current_offset = 0;

  void readImpl(char* buf, zim::zsize_t nbytes) override {
    for (size_type i=0; i<nbytes.v; i++) {
      buf[i] = (current_offset++)%256;
    }
  }

  size_t getMemorySize() const override {
    return 0;
  }
};

// ... and test that it compiles and works as intended

TEST(IStreamReader, read_zero)
{
  InfiniteZeroStream izs;
  IStreamReader& ids = izs;
  EXPECT_EQ(0, ids.read<uint32_t>());
  EXPECT_EQ(0L, ids.read<uint64_t>());

  // zim::fromLittleEndian() handles only integer types
  // EXPECT_EQ(0.0, ids.read<double>());
}

TEST(IStreamReader, sub_reader_zero)
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

TEST(IStreamReader, read_increasing)
{
  InfiniteIncreasingStream iis;
  IStreamReader& ids = iis;
  EXPECT_EQ(0x03020100, ids.read<uint32_t>());
  EXPECT_EQ(0x0B0A090807060504, ids.read<uint64_t>());

  // zim::fromLittleEndian() handles only integer types
  // EXPECT_EQ(0.0, ids.read<double>());
}

TEST(IStreamReader, sub_reader_increasing)
{
  const size_t N = 16;
  const char refbuf[N] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  InfiniteIncreasingStream iis;
  IStreamReader& ids = iis;
  auto subReader = ids.sub_reader(zim::zsize_t(N));
  EXPECT_EQ(subReader->size().v, N);
  auto buffer = subReader->get_buffer(zim::offset_t(0), zim::zsize_t(N));
  EXPECT_EQ(buffer.size().v, N);
  EXPECT_EQ(0, memcmp(buffer.data(), refbuf, N));

  buffer = subReader->get_buffer(zim::offset_t(5), zim::zsize_t(N-5));
  EXPECT_EQ(buffer.size().v, N-5);
  EXPECT_EQ(0, memcmp(buffer.data(), refbuf+5, N-5));
}


} // unnamed namespace
