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
#include <zim/fstream.h>
#include <zim/zim.h>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstdio>

#include <cxxtools/unit/testsuite.h>
#include <cxxtools/unit/registertest.h>

#include "config.h"

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
#if defined(ENABLE_BZIP2)
      registerMethod("ReadWriteClusterBz2", *this, &ClusterTest::ReadWriteClusterBz2);
#endif
#if defined(ENABLE_LZMA)
      registerMethod("ReadWriteClusterLzma", *this, &ClusterTest::ReadWriteClusterLzma);
#endif
    }

    void CreateCluster()
    {
      zim::Cluster cluster;

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
      std::string name = std::tmpnam(NULL);
      std::ofstream os;
      os.open(name.c_str());

      zim::Cluster cluster;

      std::string blob0("123456789012345678901234567890");
      std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
      std::string blob2("abcdefghijklmnopqrstuvwxyz");

      cluster.addBlob(blob0.data(), blob0.size());
      cluster.addBlob(blob1.data(), blob1.size());
      cluster.addBlob(blob2.data(), blob2.size());

      os << cluster;
      os.close();

      zim::ifstream is(name);
      zim::Cluster cluster2;
      cluster2.init_from_stream(is, 0);
      CXXTOOLS_UNIT_ASSERT(!is.fail());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.count(), 3);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(0), blob0.size());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(1), blob1.size());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(2), blob2.size());
      std::remove(name.c_str());
    }

    void ReadWriteEmpty()
    {
      std::string name = std::tmpnam(NULL);
      std::ofstream os;
      os.open(name.c_str());

      zim::Cluster cluster;

      cluster.addBlob(0, 0);
      cluster.addBlob(0, 0);
      cluster.addBlob(0, 0);

      os << cluster;
      os.close();

      zim::ifstream is(name);
      zim::Cluster cluster2;
      cluster2.init_from_stream(is, 0);
      CXXTOOLS_UNIT_ASSERT(!is.fail());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.count(), 3);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(0), 0);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(1), 0);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(2), 0);
      std::remove(name.c_str());
    }

#if defined(ENABLE_ZLIB)
    void ReadWriteClusterZ()
    {
      std::string name = std::tmpnam(NULL);
      std::ofstream os;
      os.open(name.c_str());

      zim::Cluster cluster;

      std::string blob0("123456789012345678901234567890");
      std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
      std::string blob2("abcdefghijklmnopqrstuvwxyz");

      cluster.addBlob(blob0.data(), blob0.size());
      cluster.addBlob(blob1.data(), blob1.size());
      cluster.addBlob(blob2.data(), blob2.size());
      cluster.setCompression(zim::zimcompZip);

      os << cluster;
      os.close();

      zim::ifstream is(name);
      zim::Cluster cluster2;
      cluster2.init_from_stream(is, 0);
      CXXTOOLS_UNIT_ASSERT(!is.fail());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.count(), 3);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getCompression(), zim::zimcompZip);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(0), blob0.size());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(1), blob1.size());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(2), blob2.size());
      CXXTOOLS_UNIT_ASSERT(std::equal(cluster2.getBlobPtr(0), cluster2.getBlobPtr(0) + cluster2.getBlobSize(0), blob0.data()));
      CXXTOOLS_UNIT_ASSERT(std::equal(cluster2.getBlobPtr(1), cluster2.getBlobPtr(1) + cluster2.getBlobSize(1), blob1.data()));
      CXXTOOLS_UNIT_ASSERT(std::equal(cluster2.getBlobPtr(2), cluster2.getBlobPtr(2) + cluster2.getBlobSize(2), blob2.data()));
      std::remove(name.c_str());
    }

#endif

#if defined(ENABLE_BZIP2)
    void ReadWriteClusterBz2()
    {
      std::string name = std::tmpnam(NULL);
      std::ofstream os;
      os.open(name.c_str());

      zim::Cluster cluster;

      std::string blob0("123456789012345678901234567890");
      std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
      std::string blob2("abcdefghijklmnopqrstuvwxyz");

      cluster.addBlob(blob0.data(), blob0.size());
      cluster.addBlob(blob1.data(), blob1.size());
      cluster.addBlob(blob2.data(), blob2.size());
      cluster.setCompression(zim::zimcompBzip2);

      os << cluster;
      os.close();

      zim::ifstream is(name);
      zim::Cluster cluster2;
      cluster2.init_from_stream(is, 0);
      CXXTOOLS_UNIT_ASSERT(!is.fail());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.count(), 3);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getCompression(), zim::zimcompBzip2);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(0), blob0.size());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(1), blob1.size());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(2), blob2.size());
      CXXTOOLS_UNIT_ASSERT(std::equal(cluster2.getBlobPtr(0), cluster2.getBlobPtr(0) + cluster2.getBlobSize(0), blob0.data()));
      CXXTOOLS_UNIT_ASSERT(std::equal(cluster2.getBlobPtr(1), cluster2.getBlobPtr(1) + cluster2.getBlobSize(1), blob1.data()));
      CXXTOOLS_UNIT_ASSERT(std::equal(cluster2.getBlobPtr(2), cluster2.getBlobPtr(2) + cluster2.getBlobSize(2), blob2.data()));
      std::remove(name.c_str());
    }

#endif

#if defined(ENABLE_LZMA)
    void ReadWriteClusterLzma()
    {
      std::string name = std::tmpnam(NULL);
      std::ofstream os;
      os.open(name.c_str());

      zim::Cluster cluster;

      std::string blob0("123456789012345678901234567890");
      std::string blob1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
      std::string blob2("abcdefghijklmnopqrstuvwxyz");

      cluster.addBlob(blob0.data(), blob0.size());
      cluster.addBlob(blob1.data(), blob1.size());
      cluster.addBlob(blob2.data(), blob2.size());
      cluster.setCompression(zim::zimcompLzma);

      os << cluster;
      os.close();

      zim::ifstream is(name);
      zim::Cluster cluster2;
      cluster2.init_from_stream(is, 0);
      CXXTOOLS_UNIT_ASSERT(!is.fail());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.count(), 3);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getCompression(), zim::zimcompLzma);
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(0), blob0.size());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(1), blob1.size());
      CXXTOOLS_UNIT_ASSERT_EQUALS(cluster2.getBlobSize(2), blob2.size());
      CXXTOOLS_UNIT_ASSERT(std::equal(cluster2.getBlobPtr(0), cluster2.getBlobPtr(0) + cluster2.getBlobSize(0), blob0.data()));
      CXXTOOLS_UNIT_ASSERT(std::equal(cluster2.getBlobPtr(1), cluster2.getBlobPtr(1) + cluster2.getBlobSize(1), blob1.data()));
      CXXTOOLS_UNIT_ASSERT(std::equal(cluster2.getBlobPtr(2), cluster2.getBlobPtr(2) + cluster2.getBlobSize(2), blob2.data()));
      std::remove(name.c_str());
    }

#endif

};

cxxtools::unit::RegisterTest<ClusterTest> register_ClusterTest;
