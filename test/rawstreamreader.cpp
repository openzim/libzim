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

#include "rawstreamreader.h"
#include "buffer.h"
#include "buffer_reader.h"

#include "gtest/gtest.h"

namespace
{

using namespace zim;

std::string toString(const Buffer& buffer)
{
  return std::string(buffer.data(), buffer.size().v);
}

TEST(ReaderDataStreamWrapper, shouldJustWork)
{
  char data[] = "abcdefghijklmnopqrstuvwxyz";
  toLittleEndian(uint32_t(1234), data);
  toLittleEndian(int64_t(-987654321), data+18);

  auto  reader = std::make_shared<BufferReader>(Buffer::makeBuffer(data, zsize_t(sizeof(data))));

  RawStreamReader rdr(reader);

  ASSERT_EQ(1234,         rdr.read<uint32_t>());
  auto subbuffer = rdr.sub_reader(zsize_t(4))->get_buffer(offset_t(0), zsize_t(4));
  ASSERT_EQ("efgh",       toString(subbuffer));
  subbuffer = rdr.sub_reader(zsize_t(10))->get_buffer(offset_t(0), zsize_t(10));
  ASSERT_EQ("ijklmnopqr", toString(subbuffer));
  ASSERT_EQ(-987654321,   rdr.read<int64_t>());
}

} // unnamed namespace
