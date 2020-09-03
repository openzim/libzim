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

#include "readerdatastreamwrapper.h"
#include "buffer.h"
#include "file_reader.h"

#include "gtest/gtest.h"

namespace
{

using namespace zim;

std::shared_ptr<Buffer> memoryViewBuffer(const char* str, size_t size)
{
  return std::make_shared<MemoryViewBuffer>(str, zsize_t(size));
}

std::string toString(const IDataStream::Blob& blob)
{
  return std::string(blob.data(), blob.size());
}

TEST(ReaderDataStreamWrapper, shouldJustWork)
{
  char data[] = "abcdefghijklmnopqrstuvwxyz";
  toLittleEndian(uint32_t(1234), data);
  toLittleEndian(int64_t(-987654321), data+18);

  const auto buffer = memoryViewBuffer(data, sizeof(data));
  const auto bufReader = std::make_shared<BufferReader>(buffer);
  const std::shared_ptr<const Reader> readerPtr = bufReader;

  ReaderDataStreamWrapper rdsw(readerPtr);

  ASSERT_EQ(1234,         rdsw.read<uint32_t>());
  ASSERT_EQ("efgh",       toString(rdsw.readBlob(4)));
  ASSERT_EQ("ijklmnopqr", toString(rdsw.readBlob(10)));
  ASSERT_EQ(-987654321,   rdsw.read<int64_t>());
}

} // unnamed namespace
