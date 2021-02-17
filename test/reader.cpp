/*
 * Copyright (C) 2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#include "tools.h"
#include "buffer_reader.h"
#include "file_reader.h"
#include "fs.h"
#include "file_compound.h"

#include "gtest/gtest.h"

namespace
{

using namespace zim;
using zim::unittests::makeTempFile;

////////////////////////////////////////////////////////////////////////////////
// FileReader
////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<Reader> createFileReader(const char* data, zsize_t size) {
  const auto tmpfile = makeTempFile("data", data);
  auto fd = DEFAULTFS::openFile(tmpfile->path());
  return std::unique_ptr<Reader>(new FileReader(std::make_shared<typename DEFAULTFS::FD>(std::move(fd)), offset_t(0), size));
}

std::unique_ptr<Reader> createMultiFileReader(const char* data, zsize_t size) {
  const auto tmpfile = makeTempFile("data", data);
  auto fileCompound = std::make_shared<FileCompound>(tmpfile->path());
  return std::unique_ptr<Reader>(new MultiPartFileReader(fileCompound));
}

std::unique_ptr<Reader> createBufferReader(const char* data, zsize_t size) {
  auto buffer = Buffer::makeBuffer(data, size);
  return std::unique_ptr<Reader>(new BufferReader(buffer));
}

auto createReaders = {
  createFileReader,
  createMultiFileReader,
  createBufferReader
};

TEST(FileReader, shouldJustWork)
{
  char data[] = "abcdefghijklmnopqrstuvwxyz";
  for(auto& createReader:createReaders) {
    auto baseOffset = createReader==createBufferReader ? ((offset_type)data) : 0;
    auto reader = createReader(data, zsize_t(26));

    ASSERT_EQ(offset_t(baseOffset+0), reader->offset());
    ASSERT_EQ(zsize_t(sizeof(data)-1), reader->size());

    ASSERT_EQ('a', reader->read(offset_t(0)));
    ASSERT_EQ('e', reader->read(offset_t(4)));

    char out[4] = {0, 0, 0, 0};
    reader->read(out, offset_t(0), zsize_t(4));
    ASSERT_EQ(0, memcmp(out, "abcd", 4));

    reader->read(out, offset_t(5), zsize_t(2));
    ASSERT_EQ(0, memcmp(out, "fgcd", 4));

    reader->read(out, offset_t(10), zsize_t(0));
    ASSERT_EQ(0, memcmp(out, "fgcd", 4));

    reader->read(out, offset_t(10), zsize_t(4));
    ASSERT_EQ(0, memcmp(out, "klmn", 4));

    // Can read last bit of the file.
    ASSERT_EQ('z', reader->read(offset_t(25)));
    reader->read(out, offset_t(25), zsize_t(1));
    ASSERT_EQ(0, memcmp(out, "zlmn", 4));

    // Fail if we try to read out of the file.
    ASSERT_THROW(reader->read(offset_t(26)), std::runtime_error);
    ASSERT_THROW(reader->read(out, offset_t(25), zsize_t(4)), std::runtime_error);
    ASSERT_THROW(reader->read(out, offset_t(30), zsize_t(4)), std::runtime_error);
    ASSERT_THROW(reader->read(out, offset_t(30), zsize_t(0)), std::runtime_error);
  }
}

TEST(FileReader, subReader)
{
  char data[] = "abcdefghijklmnopqrstuvwxyz";
  for(auto& createReader:createReaders) {
    auto baseOffset = createReader==createBufferReader ? ((offset_type)data) : 0;
    auto reader = createReader(data, zsize_t(26));

    auto subReader = reader->sub_reader(offset_t(4), zsize_t(20));

    ASSERT_EQ(offset_t(baseOffset+4), subReader->offset());
    ASSERT_EQ(zsize_t(20), subReader->size());

    ASSERT_EQ('e', subReader->read(offset_t(0)));
    ASSERT_EQ('i', subReader->read(offset_t(4)));

    char out[4] = {0, 0, 0, 0};
    subReader->read(out, offset_t(0), zsize_t(4));
    ASSERT_EQ(0, memcmp(out, "efgh", 4));

    subReader->read(out, offset_t(5), zsize_t(2));
    ASSERT_EQ(0, memcmp(out, "jkgh", 4));

    // Can read last bit of the file.
    ASSERT_EQ('x', subReader->read(offset_t(19)));
    subReader->read(out, offset_t(19), zsize_t(1));
    ASSERT_EQ(0, memcmp(out, "xkgh", 4));

    // Fail if we try to read out of the file.
    ASSERT_THROW(subReader->read(offset_t(20)), std::runtime_error);
    ASSERT_THROW(subReader->read(out, offset_t(18), zsize_t(4)), std::runtime_error);
    ASSERT_THROW(subReader->read(out, offset_t(30), zsize_t(4)), std::runtime_error);
    ASSERT_THROW(subReader->read(out, offset_t(30), zsize_t(0)), std::runtime_error);
  }
}

TEST(FileReader, zeroReader)
{
  char data[] = "";
  for(auto& createReader:createReaders) {
    auto baseOffset = createReader==createBufferReader ? ((offset_type)data) : 0;
    auto reader = createReader(data, zsize_t(0));

    ASSERT_EQ(offset_t(baseOffset), reader->offset());
    ASSERT_EQ(zsize_t(0), reader->size());

    // Fail if we try to read out of the file.
    ASSERT_THROW(reader->read(offset_t(0)), std::runtime_error);
    char out[4] = {0, 0, 0, 0};
    ASSERT_THROW(reader->read(out, offset_t(0), zsize_t(4)), std::runtime_error);

    // Ok to read 0 byte on a 0 sized reader
    reader->read(out, offset_t(0), zsize_t(0));
    const char nullarray[] = {0, 0, 0, 0};
    ASSERT_EQ(0, memcmp(out, nullarray, 4));
  }
}

} // unnamed namespace
