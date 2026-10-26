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

#include <fstream>
#include <cstdio>
#include <vector>

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

    // BaseFileReader always returns 0 (see file_reader.h's XXX comment);
    // only BufferReader measures its Buffer. Branch on real type, not
    // which factory built it.
    if (dynamic_cast<const BufferReader*>(reader.get())) {
      ASSERT_EQ(zsize_t(sizeof(data)-1).v, reader->getMemorySize());
    } else {
      ASSERT_EQ(0U, reader->getMemorySize());
    }

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

    ASSERT_EQ(0, memcmp(reader->get_buffer(offset_t(0), zsize_t(4)).data(), "abcd", 4));
    ASSERT_EQ(0, memcmp(reader->get_buffer(offset_t(5), zsize_t(4)).data(), "fghi", 4));
    ASSERT_EQ(0, memcmp(reader->get_buffer(offset_t(5), zsize_t(2)).data(), "fg", 2));

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

    ASSERT_EQ(0, memcmp(subReader->get_buffer(offset_t(0), zsize_t(4)).data(), "efgh", 4));
    ASSERT_EQ(0, memcmp(subReader->get_buffer(offset_t(5), zsize_t(4)).data(), "jklm", 4));
    ASSERT_EQ(0, memcmp(subReader->get_buffer(offset_t(5), zsize_t(2)).data(), "jk", 2));

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

// FileCompound needs fixed "aa"/"ab"/... suffixes, so makeTempFile()'s
// random paths don't fit; this RAII helper removes each part on destruction.
class MultiPartFiles
{
  std::vector<std::string> paths_;
public:
  ~MultiPartFiles() {
    for (auto& path : paths_) {
      std::remove(path.c_str());
    }
  }

  void addPart(const std::string& path, const std::string& content) {
    std::ofstream f(path, std::ios::binary);
    f << content;
    paths_.push_back(path);
  }
};

// Covers all access patterns against a synthetic 3-part (10 bytes each)
// compound: small/whole/boundary-straddling/large reads, aligned or not.
TEST(FileReader, multiPartReads)
{
  // TempFile's mkstemp-based naming reserves us a unique path/prefix;
  // repurpose it to place parts at <prefix>aa/<prefix>ab/<prefix>ac.
  unittests::TempFile tempFileBase("multipart");
  const std::string prefix = tempFileBase.path();

  MultiPartFiles parts;
  parts.addPart(prefix + "aa", "0123456789");
  parts.addPart(prefix + "ab", "ABCDEFGHIJ");
  parts.addPart(prefix + "ac", "KLMNOPQRST");

  auto fileCompound = std::make_shared<FileCompound>(prefix, FileCompound::MultiPartToken::Multi);
  ASSERT_TRUE(fileCompound->is_multiPart());
  MultiPartFileReader reader(fileCompound);
  ASSERT_EQ(zsize_t(30), reader.size());

  struct Case {
    const char* name;
    offset_t offset;
    zsize_t size;
    const char* expected;
  };
  const Case cases[] = {
    // Small reads (smaller than the 10-byte part size).
    {"small read strictly inside a part",             offset_t(2),  zsize_t(3),  "234"},
    {"small read aligned with a part's left boundary", offset_t(10), zsize_t(3),  "ABC"},
    {"small read aligned with a part's right boundary", offset_t(7), zsize_t(3),  "789"},
    // A whole part, read in one go.
    {"read of an entire part",                         offset_t(10), zsize_t(10), "ABCDEFGHIJ"},
    // Straddles the aa/ab boundary.
    {"read spanning one part boundary",                offset_t(8),  zsize_t(4),  "89AB"},
    // Large reads (bigger than the 10-byte part size).
    {"large read aligned with a part's beginning",     offset_t(10), zsize_t(15), "ABCDEFGHIJKLMNO"},
    {"large read aligned with a part's end",           offset_t(5),  zsize_t(15), "56789ABCDEFGHIJ"},
    {"large read extending past both boundaries of a part", offset_t(5), zsize_t(20), "56789ABCDEFGHIJKLMNO"},
  };

  for (const auto& c : cases) {
    const auto buf = reader.get_buffer(c.offset, c.size);
    EXPECT_EQ(0, memcmp(buf.data(), c.expected, c.size.v)) << c.name;
  }
}

} // unnamed namespace
