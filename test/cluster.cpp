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

#include <zim/cluster.h>
#include <zim/zim.h>
#include <zim/buffer.h>
#include <zim/file_reader.h>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <memory>
#include <cstdio>
#include <cstring>

#include "../src/writer/cluster.h"

#include <cxxtools/unit/testsuite.h>
#include <cxxtools/unit/registertest.h>

#include "../src/config.h"

class ClusterTest : public cxxtools::unit::TestSuite
{
  public:
    ClusterTest()
      : cxxtools::unit::TestSuite("zim::ClusterTest")
    {
      registerMethod("CreateCluster", *this, &ClusterTest::CreateCluster);
      registerMethod("ReadWriteCluster", *this, &ClusterTest::ReadWriteCluster);
      registerMethod("ReadWriteEmpty", *this, &ClusterTest::ReadWriteEmpty);
#if defined(ENABLE_ZLIB)
      registerMethod("ReadWriteClusterZ", *this, &ClusterTest::ReadWriteClusterZ);
#endif
#if defined(ENABLE_LZMA)
      registerMethod("ReadWriteClusterLzma", *this, &ClusterTest::ReadWriteClusterLzma);
#endif
    }

    void CreateCluster()
    {
      zim::writer::Cluster cluster;

      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster.count(), 0);

      std::string blob0("123456789012345678901234567890");
      std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
      std::string blob2("abcdefghijklmnopqrstuvwxyz");

      cluster.addBlob(blob0.data(), blob0.size());
      cluster.addBlob(blob1.data(), blob1.size());
      cluster.addBlob(blob2.data(), blob2.size());

      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster.count(), 3);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster.getBlobSize(0), blob0.size());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster.getBlobSize(1), blob1.size());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster.getBlobSize(2), blob2.size());
    }

    void ReadWriteCluster()
    {
      std::stringstream stream;

      zim::writer::Cluster cluster;

      std::string blob0("123456789012345678901234567890");
      std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
      std::string blob2("abcdefghijklmnopqrstuvwxyz");

      cluster.addBlob(blob0.data(), blob0.size());
      cluster.addBlob(blob1.data(), blob1.size());
      cluster.addBlob(blob2.data(), blob2.size());

      stream << cluster;

      std::string str_content = stream.str();
      char* content = new char[str_content.size()-1];
      memcpy(content, str_content.c_str()+1, str_content.size()-1);
      auto buffer = std::shared_ptr<zim::Buffer>(
        new zim::MemoryBuffer<true>(content, str_content.size()-1));
      auto reader = std::shared_ptr<zim::Reader>(
        new zim::BufferReader(buffer));
      zim::Cluster cluster2(reader, zim::zimcompNone);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.count(), 3);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(0), blob0.size());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(1), blob1.size());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(2), blob2.size());
    }

    void ReadWriteEmpty()
    {
      std::stringstream stream;

      zim::writer::Cluster cluster;

      cluster.addBlob(0, 0);
      cluster.addBlob(0, 0);
      cluster.addBlob(0, 0);

      stream << cluster;

      std::string str_content = stream.str();
      char* content = new char[str_content.size()-1];
      memcpy(content, str_content.c_str()+1, str_content.size()-1);
      auto buffer = std::shared_ptr<zim::Buffer>(
        new zim::MemoryBuffer<true>(content, str_content.size()-1));
      auto reader = std::shared_ptr<zim::Reader>(
        new zim::BufferReader(buffer));
      zim::Cluster cluster2(reader, zim::zimcompNone);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.count(), 3);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(0), 0);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(1), 0);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(2), 0);
    }

#if defined(ENABLE_ZLIB)
    void ReadWriteClusterZ()
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
      auto reader = std::shared_ptr<zim::Reader>(
        new zim::BufferReader(buffer));
      zim::CompressionType comp;
      std::shared_ptr<const zim::Reader> clusterReader = reader->sub_clusterReader(0, size, &comp);
      CXXTOOLS_UNIT_ASSERT_EQUALS(comp, zim::zimcompZip);
      zim::Cluster cluster2(clusterReader, comp);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.count(), 3);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getCompression(), zim::zimcompZip);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(0), blob0.size());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(1), blob1.size());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(2), blob2.size());
      CXXTOOLS_UNIT_ASSERT(std::equal(cluster2.getBlobPtr(0), cluster2.getBlobPtr(0) + cluster2.getBlobSize(0), blob0.data()));
      CXXTOOLS_UNIT_ASSERT(std::equal(cluster2.getBlobPtr(1), cluster2.getBlobPtr(1) + cluster2.getBlobSize(1), blob1.data()));
      CXXTOOLS_UNIT_ASSERT(std::equal(cluster2.getBlobPtr(2), cluster2.getBlobPtr(2) + cluster2.getBlobSize(2), blob2.data()));
    }

#endif

#if defined(ENABLE_LZMA)
    void ReadWriteClusterLzma()
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
      auto reader = std::shared_ptr<zim::Reader>(
        new zim::BufferReader(buffer));
      zim::CompressionType comp;
      std::shared_ptr<const zim::Reader> clusterReader = reader->sub_clusterReader(0, size, &comp);
      CXXTOOLS_UNIT_ASSERT_EQUALS(comp, zim::zimcompLzma);
      zim::Cluster cluster2(clusterReader, comp);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.count(), 3);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getCompression(), zim::zimcompLzma);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(0), blob0.size());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(1), blob1.size());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(2), blob2.size());
      CXXTOOLS_UNIT_ASSERT(std::equal(cluster2.getBlobPtr(0), cluster2.getBlobPtr(0) + cluster2.getBlobSize(0), blob0.data()));
      CXXTOOLS_UNIT_ASSERT(std::equal(cluster2.getBlobPtr(1), cluster2.getBlobPtr(1) + cluster2.getBlobSize(1), blob1.data()));
      CXXTOOLS_UNIT_ASSERT(std::equal(cluster2.getBlobPtr(2), cluster2.getBlobPtr(2) + cluster2.getBlobSize(2), blob2.data()));
    }

#endif

};

cxxtools::unit::RegisterTest<ClusterTest> register_ClusterTest;
