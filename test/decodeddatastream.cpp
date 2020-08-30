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

#include "decodeddatastream.h"
#include "bufdatastream.h"

#include "gtest/gtest.h"

namespace
{

template<class CompressionInfo>
std::string
compress(const std::string& data)
{
  zim::Compressor<CompressionInfo> compressor(data.size());
  compressor.init(const_cast<char*>(data.c_str()));
  compressor.feed(data.c_str(), data.size());
  zim::zsize_t comp_size;
  const auto comp_data = compressor.get_data(&comp_size);
  return std::string(comp_data.get(), comp_size.v);
}

std::string operator*(const std::string& s, unsigned N)
{
  std::string result;
  for (unsigned i=0; i<N; i++)
    result += s;
  return result;
}

std::string toString(const zim::IDataStream::Blob& blob)
{
  return std::string(blob.data(), blob.size());
}

template<typename T>
class DecodedDataStreamTest : public testing::Test {
  protected:
    typedef T CompressionInfo;
};

using CompressionTypes = ::testing::Types<
  LZMA_INFO,
  ZSTD_INFO
#if defined(ENABLE_ZLIB)
  ,ZIP_INFO
#endif
>;

TYPED_TEST_CASE(DecodedDataStreamTest, CompressionTypes);

TYPED_TEST(DecodedDataStreamTest, justCompressedData) {
  typedef typename TestFixture::CompressionInfo CompressionInfo;

  const int N = 10;
  const std::string s("DecodedDataStream should work correctly");
  const std::string compData = compress<CompressionInfo>(s*N);

  std::unique_ptr<zim::IDataStream> bds(new zim::BufDataStream(compData.data(), compData.size()));
  zim::DecodedDataStream<CompressionInfo> dds(std::move(bds), compData.size());
  for (int i=0; i<N; i++)
  {
    ASSERT_EQ(s, toString(dds.readBlob(s.size()))) << "i: " << i;
  }
}

TYPED_TEST(DecodedDataStreamTest, compressedDataFollowedByGarbage) {
  typedef typename TestFixture::CompressionInfo CompressionInfo;

  const int N = 10;
  const std::string s("DecodedDataStream should work correctly");
  const std::string compData = compress<CompressionInfo>(s*N);
  const std::string inputData = compData + std::string(10, '\0');

  std::unique_ptr<zim::IDataStream> bds(new zim::BufDataStream(inputData.data(), inputData.size()));
  zim::DecodedDataStream<CompressionInfo> dds(std::move(bds), inputData.size());
  for (int i=0; i<N; i++)
  {
    ASSERT_EQ(s, toString(dds.readBlob(s.size()))) << "i: " << i;
  }
}

} // unnamed namespace
