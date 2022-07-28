/*
 * Copyright (C) 2020 Matthieu Gautier <mgautier@kymeria.fr>
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

#include <algorithm>
#include <memory>
#include "gtest/gtest.h"

#include <zim/zim.h>

#include "../src/compression.h"

namespace
{

template<typename T>
class CompressionTest : public testing::Test {
  protected:
    typedef zim::Compressor<T> CompressorT;
    typedef zim::Uncompressor<T> DecompressorT;
};

using CompressionAlgo = ::testing::Types<
  ZSTD_INFO
>;

TYPED_TEST_CASE(CompressionTest, CompressionAlgo);

TYPED_TEST(CompressionTest, compress) {
  std::string data;
  data.reserve(100000);
  for (int i=0; i<100000; i++) {
    data.append(1, (char)(i%256));
  }
  data[99999] = 0;

  auto initialSizes = std::vector<unsigned int>{32, 1024, 1024*1024};
  auto chunkSizes = std::vector<unsigned long>{32, 512, 1024*1024};
  for (auto initialSize: initialSizes) {
    for (auto chunkSize: chunkSizes) {
      typename TestFixture::CompressorT compressor(initialSize);
      {
        bool first=true;
        unsigned long size = data.size();
        size_t offset = 0;
        while (size) {
          if (first) {
            compressor.init(const_cast<char*>(data.c_str()));
            first = false;
          }
          auto adjustedChunkSize = std::min(size, chunkSize);
          compressor.feed(data.c_str()+offset, adjustedChunkSize);
          offset += adjustedChunkSize;
          size -= adjustedChunkSize;
        }
      }

      zim::zsize_t comp_size;
      auto comp_data = compressor.get_data(&comp_size);

      typename TestFixture::DecompressorT decompressor(initialSize);
      {
        bool first=true;
        unsigned long size = comp_size.v;
        size_t offset = 0;
        while (size) {
          if (first) {
            decompressor.init(comp_data.get());
            first = false;
          }
          auto adjustedChunkSize = std::min(size, chunkSize);
          decompressor.feed(comp_data.get()+offset, adjustedChunkSize);
          offset += adjustedChunkSize;
          size -= adjustedChunkSize;
        }
      }

      zim::zsize_t decomp_size;
      auto decomp_data = decompressor.get_data(&decomp_size);

      ASSERT_EQ(decomp_size.v, data.size());
      ASSERT_EQ(data, std::string(decomp_data.get(), decomp_size.v));
    }
  }
}


}  // namespace
