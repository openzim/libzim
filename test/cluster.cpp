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
#include <zim/writer/contentProvider.h>

#include "../src/buffer.h"
#include "../src/cluster.h"
#include "../src/file_part.h"
#include "../src/file_compound.h"
#include "../src/buffer_reader.h"
#include "../src/writer/cluster.h"
#include "../src/endian_tools.h"
#include "../src/config.h"

#include "tools.h"

namespace
{

using zim::unittests::TempFile;
using zim::unittests::write_to_buffer;

TEST(ClusterTest, create_cluster)
{
  zim::writer::Cluster cluster(zim::zimcompNone);

  ASSERT_EQ(cluster.count().v, 0U);

  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnopqrstuvwxyz");

  cluster.addContent(blob0);
  cluster.addContent(blob1);
  cluster.addContent(blob2);

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

  cluster.addContent(blob0);
  cluster.addContent(blob1);
  cluster.addContent(blob2);

  cluster.close();
  auto buffer = write_to_buffer(cluster);
  const auto cluster2shptr = zim::Cluster::read(zim::BufferReader(buffer), zim::offset_t(0));
  zim::Cluster& cluster2 = *cluster2shptr;
  ASSERT_EQ(cluster2.getCompression(), zim::zimcompNone);
  ASSERT_EQ(cluster2.isExtended, false);
  ASSERT_EQ(cluster2.count().v, 3U);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(0)).v, blob0.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(1)).v, blob1.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(2)).v, blob2.size());
}

TEST(ClusterTest, read_write_empty)
{
  zim::writer::Cluster cluster(zim::zimcompNone);

  std::string emptyString;

  cluster.addContent(emptyString);
  cluster.addContent(emptyString);
  cluster.addContent(emptyString);

  cluster.close();
  auto buffer = write_to_buffer(cluster);
  const auto cluster2shptr = zim::Cluster::read(zim::BufferReader(buffer), zim::offset_t(0));
  zim::Cluster& cluster2 = *cluster2shptr;
  ASSERT_EQ(cluster2.getCompression(), zim::zimcompNone);
  ASSERT_EQ(cluster2.isExtended, false);
  ASSERT_EQ(cluster2.count().v, 3U);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(0)).v, 0U);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(1)).v, 0U);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(2)).v, 0U);
}

TEST(ClusterTest, read_write_clusterLzma)
{
  zim::writer::Cluster cluster(zim::zimcompLzma);

  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnopqrstuvwxyz");

  cluster.addContent(blob0);
  cluster.addContent(blob1);
  cluster.addContent(blob2);

  cluster.close();
  auto buffer = write_to_buffer(cluster);
  const auto cluster2shptr = zim::Cluster::read(zim::BufferReader(buffer), zim::offset_t(0));
  zim::Cluster& cluster2 = *cluster2shptr;
  ASSERT_EQ(cluster2.isExtended, false);
  ASSERT_EQ(cluster2.count().v, 3U);
  ASSERT_EQ(cluster2.getCompression(), zim::zimcompLzma);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(0)).v, blob0.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(1)).v, blob1.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(2)).v, blob2.size());
  ASSERT_EQ(blob0, std::string(cluster2.getBlob(zim::blob_index_t(0))));
  ASSERT_EQ(blob1, std::string(cluster2.getBlob(zim::blob_index_t(1))));
  ASSERT_EQ(blob2, std::string(cluster2.getBlob(zim::blob_index_t(2))));
}

TEST(ClusterTest, read_write_clusterZstd)
{
  zim::writer::Cluster cluster(zim::zimcompZstd);

  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnopqrstuvwxyz");

  cluster.addContent(blob0);
  cluster.addContent(blob1);
  cluster.addContent(blob2);

  cluster.close();
  auto buffer = write_to_buffer(cluster);
  const auto cluster2shptr = zim::Cluster::read(zim::BufferReader(buffer), zim::offset_t(0));
  zim::Cluster& cluster2 = *cluster2shptr;
  ASSERT_EQ(cluster2.isExtended, false);
  ASSERT_EQ(cluster2.count().v, 3U);
  ASSERT_EQ(cluster2.getCompression(), zim::zimcompZstd);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(0)).v, blob0.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(1)).v, blob1.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(2)).v, blob2.size());
  ASSERT_EQ(blob0, std::string(cluster2.getBlob(zim::blob_index_t(0))));
  ASSERT_EQ(blob1, std::string(cluster2.getBlob(zim::blob_index_t(1))));
  ASSERT_EQ(blob2, std::string(cluster2.getBlob(zim::blob_index_t(2))));
}

class FakeProvider : public zim::writer::ContentProvider
{
  public:
    FakeProvider(zim::size_type size)
      : size(size),
        offset(0),
        buffer(new char[1024*1024U])
    {
      memset(buffer.get(), 0, 1024*1024U);
    }

    zim::size_type getSize() const { return size; }
    zim::Blob feed() {
      auto outSize = std::min(zim::size_type(1024*1024), size-offset);
      auto blob = zim::Blob(buffer.get(), outSize);
      offset += outSize;
      return blob;
    }

  private:
    zim::size_type size;
    zim::offset_type offset;
    std::unique_ptr<char[]> buffer;
};

TEST(ClusterTest, read_write_extended_cluster)
{
  //zim::writer doesn't suport 32 bits architectures.
  if (SIZE_MAX == UINT32_MAX) {
    return;
  }

  char* SKIP_BIG_MEMORY_TEST = std::getenv("SKIP_BIG_MEMORY_TEST");
  if (SKIP_BIG_MEMORY_TEST != nullptr && std::string(SKIP_BIG_MEMORY_TEST) == "1") {
    std::cout << "Skip big memory test" << std::endl;
    return;
  }

  // MEM = 0
  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnopqrstuvwxyz");
  zim::size_type bigger_than_4g = 1024LL*1024LL*1024LL*4LL+1024LL;
  auto bigProvider = std::unique_ptr<zim::writer::ContentProvider>(new FakeProvider(bigger_than_4g));

  zim::writer::Cluster cluster(zim::zimcompNone);
  cluster.addContent(blob0);
  cluster.addContent(blob1);
  cluster.addContent(blob2);
  cluster.addContent(std::move(bigProvider));

  ASSERT_EQ(cluster.is_extended(), true);

  auto buffer = write_to_buffer(cluster);
  // 4GiB

  const auto cluster2shptr = zim::Cluster::read(zim::BufferReader(buffer), zim::offset_t(0));
  zim::Cluster& cluster2 = *cluster2shptr;
  ASSERT_EQ(cluster2.isExtended, true);
  ASSERT_EQ(cluster2.count().v, 4U);
  ASSERT_EQ(cluster2.getCompression(), zim::zimcompNone);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(0)).v, blob0.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(1)).v, blob1.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(2)).v, blob2.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(3)).v, bigger_than_4g);
  ASSERT_EQ(blob0, std::string(cluster2.getBlob(zim::blob_index_t(0))));
  ASSERT_EQ(blob1, std::string(cluster2.getBlob(zim::blob_index_t(1))));
  ASSERT_EQ(blob2, std::string(cluster2.getBlob(zim::blob_index_t(2))));
}


TEST(ClusterTest, read_extended_cluster)
{
  char* SKIP_BIG_MEMORY_TEST = std::getenv("SKIP_BIG_MEMORY_TEST");
  if (SKIP_BIG_MEMORY_TEST != nullptr && std::string(SKIP_BIG_MEMORY_TEST) == "1") {
    std::cout << "Skip big memory test" << std::endl;
    return;
  }

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

  auto fileCompound = std::make_shared<zim::FileCompound>(fd);
  const auto cluster2shptr = zim::Cluster::read(zim::MultiPartFileReader(fileCompound), zim::offset_t(0));
  zim::Cluster& cluster2 = *cluster2shptr;
  ASSERT_EQ(cluster2.isExtended, true);
  ASSERT_EQ(cluster2.count().v, 4U);
  ASSERT_EQ(cluster2.getCompression(), zim::zimcompNone);
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(0)).v, blob0.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(1)).v, blob1.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(2)).v, blob2.size());
  ASSERT_EQ(cluster2.getBlobSize(zim::blob_index_t(3)).v, bigger_than_4g);


  ASSERT_EQ(blob0, std::string(cluster2.getBlob(zim::blob_index_t(0))));
  ASSERT_EQ(blob1, std::string(cluster2.getBlob(zim::blob_index_t(1))));
  ASSERT_EQ(blob2, std::string(cluster2.getBlob(zim::blob_index_t(2))));

  const zim::Blob b = cluster2.getBlob(zim::blob_index_t(3));
  if (SIZE_MAX == UINT32_MAX) {
    ASSERT_EQ(b.data(), nullptr);
    ASSERT_EQ(b.size(), 0U);
  } else {
    ASSERT_EQ(b.size(), bigger_than_4g);
  }

  fclose(tmpfile);
}


}  // namespace
