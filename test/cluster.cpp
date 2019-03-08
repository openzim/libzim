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

#include <zim/zim.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#if defined(_MSC_VER)
# include <BaseTsd.h>
  typedef SSIZE_T ssize_t;
#else
# include <unistd.h>
#endif

#include "gtest/gtest.h"

#include "../src/buffer.h"
#include "../src/cluster.h"
#include "../src/file_part.h"
#include "../src/file_compound.h"
#include "../src/file_reader.h"
#include "../src/writer/cluster.h"
#include "../src/endian_tools.h"

#include "../src/config.h"

namespace
{
TEST(ClusterTest, create_cluster)
{
  zim::writer::Cluster cluster(zim::zimcompNone);

  ASSERT_EQ(cluster.count().v, 0U);

  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnopqrstuvwxyz");

  cluster.addData(blob0.data(), zim::zsize_t(blob0.size()));
  cluster.addData(blob1.data(), zim::zsize_t(blob1.size()));
  cluster.addData(blob2.data(), zim::zsize_t(blob2.size()));

  ASSERT_EQ(cluster.count().v, 3U);
  ASSERT_EQ(cluster.getBlobSize(zim::blob_index_t(0)).v, blob0.size());
  ASSERT_EQ(cluster.getBlobSize(zim::blob_index_t(1)).v, blob1.size());
  ASSERT_EQ(cluster.getBlobSize(zim::blob_index_t(2)).v, blob2.size());
}

TEST(ClusterTest, read_write_cluster)
{
  std::stringstream stream;

  zim::writer::Cluster cluster(zim::zimcompNone);

  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnop vwxyz");

  cluster.addData(blob0.data(), zim::zsize_t(blob0.size()));
  cluster.addData(blob1.data(), zim::zsize_t(blob1.size()));
  cluster.addData(blob2.data(), zim::zsize_t(blob2.size()));

  cluster.dump(stream);

  std::string str_content = stream.str();
  char* content = new char[str_content.size() - 1];
  memcpy(content, str_content.c_str() + 1, str_content.size() - 1);
  auto buffer = std::shared_ptr<zim::Buffer>(
      new zim::MemoryBuffer<true>(content, zim::zsize_t(str_content.size() - 1)));
  auto reader = std::shared_ptr<zim::Reader>(new zim::BufferReader(buffer));
  zim::Cluster cluster2(reader, zim::zimcompNone, false);
  ASSERT_EQ(cluster2.count().v, 3U);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(0)).v, blob0.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(1)).v, blob1.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(2)).v, blob2.size());
}

TEST(ClusterTest, read_write_empty)
{
  std::stringstream stream;

  zim::writer::Cluster cluster(zim::zimcompNone);

  cluster.addData(0, zim::zsize_t(0));
  cluster.addData(0, zim::zsize_t(0));
  cluster.addData(0, zim::zsize_t(0));

  cluster.dump(stream);

  std::string str_content = stream.str();
  char* content = new char[str_content.size() - 1];
  memcpy(content, str_content.c_str() + 1, str_content.size() - 1);
  auto buffer = std::shared_ptr<zim::Buffer>(
      new zim::MemoryBuffer<true>(content, zim::zsize_t(str_content.size() - 1)));
  auto reader = std::shared_ptr<zim::Reader>(new zim::BufferReader(buffer));
  zim::Cluster cluster2(reader, zim::zimcompNone, false);
  ASSERT_EQ(cluster2.count().v, 3U);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(0)).v, 0U);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(1)).v, 0U);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(2)).v, 0U);
}

#if defined(ENABLE_ZLIB)
TEST(ClusterTest, read_write_clusterZ)
{
  std::stringstream stream;

  zim::writer::Cluster cluster(zim::zimcompZip);

  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnopqrstuvwxyz");

  cluster.addData(blob0.data(), zim::zsize_t(blob0.size()));
  cluster.addData(blob1.data(), zim::zsize_t(blob1.size()));
  cluster.addData(blob2.data(), zim::zsize_t(blob2.size()));

  cluster.dump(stream);

  std::string str_content = stream.str();
  int size = str_content.size();
  char* content = new char[size];
  memcpy(content, str_content.c_str(), size);
  auto buffer = std::shared_ptr<zim::Buffer>(
      new zim::MemoryBuffer<true>(content, zim::zsize_t(size)));
  auto reader = std::shared_ptr<zim::Reader>(new zim::BufferReader(buffer));
  zim::CompressionType comp;
  bool extended;
  std::shared_ptr<const zim::Reader> clusterReader
      = reader->sub_clusterReader(zim::offset_t(0), &comp, &extended);
  ASSERT_EQ(comp, zim::zimcompZip);
  ASSERT_EQ(extended, false);
  zim::Cluster cluster2(clusterReader, comp, extended);
  ASSERT_EQ(cluster2.count().v, 3U);
  ASSERT_EQ(cluster2.getCompression(), zim::zimcompZip);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(0)).v, blob0.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(1)).v, blob1.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(2)).v, blob2.size());
  auto b = cluster2.getBlob(zim::blob_index_t(0));
  ASSERT_TRUE(std::equal(b.data(), b.end(), blob0.data()));
  b = cluster2.getBlob(zim::blob_index_t(1));
  ASSERT_TRUE(std::equal(b.data(), b.end(), blob1.data()));
  b = cluster2.getBlob(zim::blob_index_t(2));
  ASSERT_TRUE(std::equal(b.data(), b.end(), blob2.data()));
}

#endif

TEST(ClusterTest, read_write_clusterLzma)
{
  std::stringstream stream;

  zim::writer::Cluster cluster(zim::zimcompLzma);

  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnopqrstuvwxyz");

  cluster.addData(blob0.data(), zim::zsize_t(blob0.size()));
  cluster.addData(blob1.data(), zim::zsize_t(blob1.size()));
  cluster.addData(blob2.data(), zim::zsize_t(blob2.size()));

  cluster.dump(stream);

  std::string str_content = stream.str();
  int size = str_content.size();
  char* content = new char[size];
  memcpy(content, str_content.c_str(), size);
  auto buffer = std::shared_ptr<zim::Buffer>(
      new zim::MemoryBuffer<true>(content, zim::zsize_t(size)));
  auto reader = std::shared_ptr<zim::Reader>(new zim::BufferReader(buffer));
  zim::CompressionType comp;
  bool extended;
  std::shared_ptr<const zim::Reader> clusterReader
      = reader->sub_clusterReader(zim::offset_t(0), &comp, &extended);
  ASSERT_EQ(comp, zim::zimcompLzma);
  ASSERT_EQ(extended, false);
  zim::Cluster cluster2(clusterReader, comp, extended);
  ASSERT_EQ(cluster2.count().v, 3U);
  ASSERT_EQ(cluster2.getCompression(), zim::zimcompLzma);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(0)).v, blob0.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(1)).v, blob1.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(2)).v, blob2.size());
  auto b = cluster2.getBlob(zim::blob_index_t(0));
  ASSERT_TRUE(std::equal(b.data(), b.end(), blob0.data()));
  b = cluster2.getBlob(zim::blob_index_t(1));
  ASSERT_TRUE(std::equal(b.data(), b.end(), blob1.data()));
  b = cluster2.getBlob(zim::blob_index_t(2));
  ASSERT_TRUE(std::equal(b.data(), b.end(), blob2.data()));
}

#if !defined(__APPLE__)
TEST(CluterTest, read_write_extended_cluster)
{
  //zim::writer doesn't suport 32 bits architectures.
  if (SIZE_MAX == UINT32_MAX) {
    return;
  }

  char* SKIP_BIG_MEMORY_TEST = std::getenv("SKIP_BIG_MEMORY_TEST");
  if (SKIP_BIG_MEMORY_TEST != nullptr && std::string(SKIP_BIG_MEMORY_TEST) == "1") {
    return;
  }

  // MEM = 0
  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnopqrstuvwxyz");
  zim::size_type bigger_than_4g = 1024LL*1024LL*1024LL*4LL+1024LL;

  std::string str_content;
  {
    std::stringstream stream;

    char* blob3 = nullptr;
    try {
      blob3 = new char[bigger_than_4g];
      // MEM = 4GiB
    } catch (std::bad_alloc& e) {
      // Not enough memory, we cannot test cluster bigger than 4Go :(
      return;
    }

    {
      zim::writer::Cluster cluster(zim::zimcompNone);
      cluster.addData(blob0.data(), zim::zsize_t(blob0.size()));
      cluster.addData(blob1.data(), zim::zsize_t(blob1.size()));
      cluster.addData(blob2.data(), zim::zsize_t(blob2.size()));
      try {
        cluster.addData(blob3, zim::zsize_t(bigger_than_4g));
        // MEM = 8GiB
      } catch (std::bad_alloc& e) {
        // Not enough memory, we cannot test cluster bigger than 4Go :(
        delete[] blob3;
        return;
      }
      ASSERT_EQ(cluster.is_extended(), true);

      delete[] blob3;
      // MEM = 4GiB

      // This is a nasty hack.
      // We preallocate the stream internal buffer to handle the content of our
      // cluster. Without this, stringstream will reallocate its internal
      // buffer as needed. As it tries to preallocate more memory than needed,
      // it ends to allocate 12GiB. Plus our 4GiB cluster, we end to need 16GiB
      // of memory.
      // By allocating a buffer bigger than what we need (but not to big),
      // we avoid a too big allocation and limit our global need to 12GiB.
      // We would be nice to avoid the creation of the tmp string and limit
      // ourselves to a global allocation of 8GiB but I don't know how to do
      // it without creating a custom streambuf :/
      {
        std::string tmp;
        tmp.reserve(bigger_than_4g + 1024LL);
        // MEM = 8GiB
        tmp.resize(bigger_than_4g + 1024LL);
        stream.str(tmp);
        // MEM = 12GiB
        stream.str("");
      }
      // MEM = 8GiB (tmp is deleted)
      cluster.dump(stream);
      // MEM = 8GiB
    }
    // MEM = 4GiB (cluster is deleted)

    str_content = stream.str();
    // MEM = 8GiB
  }
  // MEM = 4 GiB (stream is deleted)
  auto size = str_content.size();
  auto buffer = std::shared_ptr<zim::Buffer>(
    new zim::MemoryBuffer<false>(str_content.data(), zim::zsize_t(size)));
  auto reader = std::shared_ptr<zim::Reader>(new zim::BufferReader(buffer));
  zim::CompressionType comp;
  bool extended;
  std::shared_ptr<const zim::Reader> clusterReader
      = reader->sub_clusterReader(zim::offset_t(0), &comp, &extended);
  ASSERT_EQ(extended, true);
  zim::Cluster cluster2(clusterReader, comp, extended);
  ASSERT_EQ(cluster2.count().v, 4U);
  ASSERT_EQ(cluster2.getCompression(), zim::zimcompNone);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(0)).v, blob0.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(1)).v, blob1.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(2)).v, blob2.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(3)).v, bigger_than_4g);
  auto b = cluster2.getBlob(zim::blob_index_t(0));
  ASSERT_TRUE(std::equal(b.data(), b.end(), blob0.data()));
  b = cluster2.getBlob(zim::blob_index_t(1));
  ASSERT_TRUE(std::equal(b.data(), b.end(), blob1.data()));
  b = cluster2.getBlob(zim::blob_index_t(2));
  ASSERT_TRUE(std::equal(b.data(), b.end(), blob2.data()));
}
#endif

TEST(CluterTest, read_extended_cluster)
{
  std::FILE* tmpfile = std::tmpfile();
  int fd = fileno(tmpfile);
  ssize_t bytes_written;

  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnopqrstuvwxyz");

  zim::size_type bigger_than_4g = 1024LL*1024LL*1024LL*4LL+1024LL;

  zim::offset_type offset = 5*sizeof(uint64_t);

  char a = 0x11;
  bytes_written = write(fd, &a, 1);

  char out_buf[sizeof(uint64_t)];

  zim::toLittleEndian(offset, out_buf);
  bytes_written = write(fd, out_buf, sizeof(uint64_t));

  offset += blob0.size();
  zim::toLittleEndian(offset, out_buf);
  bytes_written = write(fd, out_buf, sizeof(uint64_t));

  offset += blob1.size();
  zim::toLittleEndian(offset, out_buf);
  bytes_written = write(fd, out_buf, sizeof(uint64_t));

  offset += blob2.size();
  zim::toLittleEndian(offset, out_buf);
  bytes_written = write(fd, out_buf, sizeof(uint64_t));

  offset += bigger_than_4g;
  zim::toLittleEndian(offset, out_buf);
  bytes_written = write(fd, out_buf, sizeof(uint64_t));

  bytes_written = write(fd, blob0.c_str(), blob0.size());
  ASSERT_EQ(bytes_written, (ssize_t)blob0.size());

  bytes_written = write(fd, blob1.c_str(), blob1.size());
  ASSERT_EQ(bytes_written, (ssize_t)blob1.size());

  bytes_written = write(fd, blob2.c_str(), blob2.size());
  ASSERT_EQ(bytes_written, (ssize_t)blob2.size());

#ifdef _WIN32
# define LSEEK _lseeki64
#else
# define LSEEK lseek
#endif
  LSEEK(fd , bigger_than_4g-1, SEEK_CUR);
#undef LSEEK
//  std::fseek(tmpfile, bigger_than_4g-1, SEEK_CUR);
  a = '\0';
  bytes_written = write(fd, &a, 1);
  fflush(tmpfile);

  auto filePart = new zim::FilePart<>(fileno(tmpfile));
  auto fileCompound = std::shared_ptr<zim::FileCompound>(new zim::FileCompound(filePart));
  auto reader = std::shared_ptr<zim::Reader>(new zim::FileReader(fileCompound));
  zim::CompressionType comp;
  bool extended;
  std::shared_ptr<const zim::Reader> clusterReader
      = reader->sub_clusterReader(zim::offset_t(0), &comp, &extended);
  ASSERT_EQ(extended, true);
  zim::Cluster cluster2(clusterReader, comp, extended);
  ASSERT_EQ(cluster2.count().v, 4U);
  ASSERT_EQ(cluster2.getCompression(), zim::zimcompNone);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(0)).v, blob0.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(1)).v, blob1.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(2)).v, blob2.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(3)).v, bigger_than_4g);


  auto b = cluster2.getBlob(zim::blob_index_t(0));
  ASSERT_TRUE(std::equal(b.data(), b.end(), blob0.data()));
  b = cluster2.getBlob(zim::blob_index_t(1));
  ASSERT_TRUE(std::equal(b.data(), b.end(), blob1.data()));
  b = cluster2.getBlob(zim::blob_index_t(2));
  ASSERT_TRUE(std::equal(b.data(), b.end(), blob2.data()));

  b = cluster2.getBlob(zim::blob_index_t(3));
  if (SIZE_MAX == UINT32_MAX) {
    ASSERT_EQ(b.data(), nullptr);
    ASSERT_EQ(b.size(), 0U);
  } else {
    ASSERT_EQ(b.size(), bigger_than_4g);
  }

  fclose(tmpfile);
}


}  // namespace

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
