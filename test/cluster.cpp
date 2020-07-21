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

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#if defined(_MSC_VER)
# include <BaseTsd.h>
  typedef SSIZE_T ssize_t;
#else
# include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <fileapi.h>
#undef min
#undef max
#endif

#include "gtest/gtest.h"

#include <zim/zim.h>

#include "../src/buffer.h"
#include "../src/cluster.h"
#include "../src/file_part.h"
#include "../src/file_compound.h"
#include "../src/file_reader.h"
#include "../src/writer/cluster.h"
#include "../src/endian_tools.h"
#include "../src/config.h"

#include "tempfile.h"

namespace
{

using zim::unittests::TempFile;

std::shared_ptr<zim::Buffer> write_to_buffer(zim::writer::Cluster& cluster)
{
  const TempFile tmpFile("test_cluster");
  cluster.close();
  const auto tmp_fd = tmpFile.fd();
  cluster.write(tmp_fd);
  auto size = lseek(tmp_fd, 0, SEEK_END);

  char* content = new char[size];
  lseek(tmp_fd, 0, SEEK_SET);
  if (read(tmp_fd, content, size) == -1)
    throw std::runtime_error("Cannot read");
  return std::shared_ptr<zim::Buffer>(
      new zim::MemoryBuffer<true>(content, zim::zsize_t(size)));
}

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
  zim::writer::Cluster cluster(zim::zimcompNone);

  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnop vwxyz");

  cluster.addData(blob0.data(), zim::zsize_t(blob0.size()));
  cluster.addData(blob1.data(), zim::zsize_t(blob1.size()));
  cluster.addData(blob2.data(), zim::zsize_t(blob2.size()));

  auto buffer = write_to_buffer(cluster);
  zim::CompressionType comp;
  bool extended;
  auto reader = std::shared_ptr<const zim::Reader>(zim::BufferReader(buffer).sub_clusterReader(zim::offset_t(0), &comp, &extended));
  zim::Cluster cluster2(reader, zim::zimcompNone, false);
  ASSERT_EQ(cluster2.count().v, 3U);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(0)).v, blob0.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(1)).v, blob1.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(2)).v, blob2.size());
}

TEST(ClusterTest, read_write_empty)
{
  zim::writer::Cluster cluster(zim::zimcompNone);

  cluster.addData(0, zim::zsize_t(0));
  cluster.addData(0, zim::zsize_t(0));
  cluster.addData(0, zim::zsize_t(0));

  auto buffer = write_to_buffer(cluster);
  zim::CompressionType comp;
  bool extended;
  auto reader = std::shared_ptr<const zim::Reader>(zim::BufferReader(buffer).sub_clusterReader(zim::offset_t(0), &comp, &extended));
  zim::Cluster cluster2(reader, zim::zimcompNone, false);
  ASSERT_EQ(cluster2.count().v, 3U);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(0)).v, 0U);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(1)).v, 0U);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(2)).v, 0U);
}

#if defined(ENABLE_ZLIB)
TEST(ClusterTest, read_write_clusterZ)
{
  zim::writer::Cluster cluster(zim::zimcompZip);

  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnopqrstuvwxyz");

  cluster.addData(blob0.data(), zim::zsize_t(blob0.size()));
  cluster.addData(blob1.data(), zim::zsize_t(blob1.size()));
  cluster.addData(blob2.data(), zim::zsize_t(blob2.size()));

  auto buffer = write_to_buffer(cluster);
  zim::CompressionType comp;
  bool extended;
  auto reader = std::shared_ptr<const zim::Reader>(zim::BufferReader(buffer).sub_clusterReader(zim::offset_t(0), &comp, &extended));
  ASSERT_EQ(comp, zim::zimcompZip);
  ASSERT_EQ(extended, false);
  zim::Cluster cluster2(reader, comp, extended);
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
  zim::writer::Cluster cluster(zim::zimcompLzma);

  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnopqrstuvwxyz");

  cluster.addData(blob0.data(), zim::zsize_t(blob0.size()));
  cluster.addData(blob1.data(), zim::zsize_t(blob1.size()));
  cluster.addData(blob2.data(), zim::zsize_t(blob2.size()));

  auto buffer = write_to_buffer(cluster);
  zim::CompressionType comp;
  bool extended;
  auto reader = std::shared_ptr<const zim::Reader>(zim::BufferReader(buffer).sub_clusterReader(zim::offset_t(0), &comp, &extended));
  ASSERT_EQ(comp, zim::zimcompLzma);
  ASSERT_EQ(extended, false);
  zim::Cluster cluster2(reader, comp, extended);
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

TEST(ClusterTest, read_write_clusterZstd)
{
  zim::writer::Cluster cluster(zim::zimcompZstd);

  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnopqrstuvwxyz");

  cluster.addData(blob0.data(), zim::zsize_t(blob0.size()));
  cluster.addData(blob1.data(), zim::zsize_t(blob1.size()));
  cluster.addData(blob2.data(), zim::zsize_t(blob2.size()));

  auto buffer = write_to_buffer(cluster);
  zim::CompressionType comp;
  bool extended;
  auto reader = std::shared_ptr<const zim::Reader>(zim::BufferReader(buffer).sub_clusterReader(zim::offset_t(0), &comp, &extended));
  ASSERT_EQ(comp, zim::zimcompZstd);
  ASSERT_EQ(extended, false);
  zim::Cluster cluster2(reader, comp, extended);
  ASSERT_EQ(cluster2.count().v, 3U);
  ASSERT_EQ(cluster2.getCompression(), zim::zimcompZstd);
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
TEST(ClusterTest, read_write_extended_cluster)
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

  std::shared_ptr<zim::Buffer> buffer;
  {
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

      buffer = write_to_buffer(cluster);
    }
  }
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

TEST(ClusterTest, read_extended_cluster)
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
