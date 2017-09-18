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

#include "gtest/gtest.h"

#include "../src/buffer.h"
#include "../src/cluster.h"
#include "../src/file_reader.h"
#include "../src/writer/cluster.h"

#include "../src/config.h"

namespace
{
TEST(ClusterTest, create_cluster)
{
  zim::writer::Cluster cluster;

  ASSERT_EQ(cluster.count(), 0);

  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnopqrstuvwxyz");

  cluster.addBlob(blob0.data(), blob0.size());
  cluster.addBlob(blob1.data(), blob1.size());
  cluster.addBlob(blob2.data(), blob2.size());

  ASSERT_EQ(cluster.count(), 3);
  ASSERT_EQ(cluster.getBlobSize(0), blob0.size());
  ASSERT_EQ(cluster.getBlobSize(1), blob1.size());
  ASSERT_EQ(cluster.getBlobSize(2), blob2.size());
}

TEST(ClusterTest, read_write_cluster)
{
  std::stringstream stream;

  zim::writer::Cluster cluster;

  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnop vwxyz");

  cluster.addBlob(blob0.data(), blob0.size());
  cluster.addBlob(blob1.data(), blob1.size());
  cluster.addBlob(blob2.data(), blob2.size());

  stream << cluster;

  std::string str_content = stream.str();
  char* content = new char[str_content.size() - 1];
  memcpy(content, str_content.c_str() + 1, str_content.size() - 1);
  auto buffer = std::shared_ptr<zim::Buffer>(
      new zim::MemoryBuffer<true>(content, str_content.size() - 1));
  auto reader = std::shared_ptr<zim::Reader>(new zim::BufferReader(buffer));
  zim::Cluster cluster2(reader, zim::zimcompNone);
  ASSERT_EQ(cluster2.count(), 3);
  ASSERT_EQ(cluster2.getBlobSize(0), blob0.size());
  ASSERT_EQ(cluster2.getBlobSize(1), blob1.size());
  ASSERT_EQ(cluster2.getBlobSize(2), blob2.size());
}

TEST(ClusterTest, read_write_empty)
{
  std::stringstream stream;

  zim::writer::Cluster cluster;

  cluster.addBlob(0, 0);
  cluster.addBlob(0, 0);
  cluster.addBlob(0, 0);

  stream << cluster;

  std::string str_content = stream.str();
  char* content = new char[str_content.size() - 1];
  memcpy(content, str_content.c_str() + 1, str_content.size() - 1);
  auto buffer = std::shared_ptr<zim::Buffer>(
      new zim::MemoryBuffer<true>(content, str_content.size() - 1));
  auto reader = std::shared_ptr<zim::Reader>(new zim::BufferReader(buffer));
  zim::Cluster cluster2(reader, zim::zimcompNone);
  ASSERT_EQ(cluster2.count(), 3);
  ASSERT_EQ(cluster2.getBlobSize(0), 0);
  ASSERT_EQ(cluster2.getBlobSize(1), 0);
  ASSERT_EQ(cluster2.getBlobSize(2), 0);
}

#if defined(ENABLE_ZLIB)
TEST(ClusterTest, read_write_clusterZ)
{
  std::stringstream stream;

  zim::writer::Cluster cluster;

  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnopqrstuvwxyz");

  cluster.addBlob(blob0.data(), blob0.size());
  cluster.addBlob(blob1.data(), blob1.size());
  cluster.addBlob(blob2.data(), blob2.size());
  cluster.setCompression(zim::zimcompZip);

  stream << cluster;

  std::string str_content = stream.str();
  int size = str_content.size();
  char* content = new char[size];
  memcpy(content, str_content.c_str(), size);
  auto buffer = std::shared_ptr<zim::Buffer>(
      new zim::MemoryBuffer<true>(content, size));
  auto reader = std::shared_ptr<zim::Reader>(new zim::BufferReader(buffer));
  zim::CompressionType comp;
  std::shared_ptr<const zim::Reader> clusterReader
      = reader->sub_clusterReader(0, size, &comp);
  ASSERT_EQ(comp, zim::zimcompZip);
  zim::Cluster cluster2(clusterReader, comp);
  ASSERT_EQ(cluster2.count(), 3);
  ASSERT_EQ(cluster2.getCompression(), zim::zimcompZip);
  ASSERT_EQ(cluster2.getBlobSize(0), blob0.size());
  ASSERT_EQ(cluster2.getBlobSize(1), blob1.size());
  ASSERT_EQ(cluster2.getBlobSize(2), blob2.size());
  ASSERT_TRUE(std::equal(cluster2.getBlobPtr(0),
                         cluster2.getBlobPtr(0) + cluster2.getBlobSize(0),
                         blob0.data()));
  ASSERT_TRUE(std::equal(cluster2.getBlobPtr(1),
                         cluster2.getBlobPtr(1) + cluster2.getBlobSize(1),
                         blob1.data()));
  ASSERT_TRUE(std::equal(cluster2.getBlobPtr(2),
                         cluster2.getBlobPtr(2) + cluster2.getBlobSize(2),
                         blob2.data()));
}

#endif

#if defined(ENABLE_LZMA)
TEST(ClusterTest, read_write_clusterLzma)
{
  std::stringstream stream;

  zim::writer::Cluster cluster;

  std::string blob0("123456789012345678901234567890");
  std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  std::string blob2("abcdefghijklmnopqrstuvwxyz");

  cluster.addBlob(blob0.data(), blob0.size());
  cluster.addBlob(blob1.data(), blob1.size());
  cluster.addBlob(blob2.data(), blob2.size());
  cluster.setCompression(zim::zimcompLzma);

  stream << cluster;

  std::string str_content = stream.str();
  int size = str_content.size();
  char* content = new char[size];
  memcpy(content, str_content.c_str(), size);
  auto buffer = std::shared_ptr<zim::Buffer>(
      new zim::MemoryBuffer<true>(content, size));
  auto reader = std::shared_ptr<zim::Reader>(new zim::BufferReader(buffer));
  zim::CompressionType comp;
  std::shared_ptr<const zim::Reader> clusterReader
      = reader->sub_clusterReader(0, size, &comp);
  ASSERT_EQ(comp, zim::zimcompLzma);
  zim::Cluster cluster2(clusterReader, comp);
  ASSERT_EQ(cluster2.count(), 3);
  ASSERT_EQ(cluster2.getCompression(), zim::zimcompLzma);
  ASSERT_EQ(cluster2.getBlobSize(0), blob0.size());
  ASSERT_EQ(cluster2.getBlobSize(1), blob1.size());
  ASSERT_EQ(cluster2.getBlobSize(2), blob2.size());
  ASSERT_TRUE(std::equal(cluster2.getBlobPtr(0),
                         cluster2.getBlobPtr(0) + cluster2.getBlobSize(0),
                         blob0.data()));
  ASSERT_TRUE(std::equal(cluster2.getBlobPtr(1),
                         cluster2.getBlobPtr(1) + cluster2.getBlobSize(1),
                         blob1.data()));
  ASSERT_TRUE(std::equal(cluster2.getBlobPtr(2),
                         cluster2.getBlobPtr(2) + cluster2.getBlobSize(2),
                         blob2.data()));
}

#endif

}  // namespace

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
