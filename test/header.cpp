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

#include <iostream>
#include <sstream>
#include <zim/fileheader.h>

#include <cxxtools/unit/testsuite.h>
#include <cxxtools/unit/registertest.h>

class FileheaderTest : public cxxtools::unit::TestSuite
{
  public:
    FileheaderTest()
      : cxxtools::unit::TestSuite("zim::FileheaderTest")
    {
      registerMethod("ReadWriteHeader", *this, &FileheaderTest::ReadWriteHeader);
    }

    void ReadWriteHeader()
    {
      zim::Fileheader header;
      header.setUuid("1234567890abcdef");
      header.setArticleCount(4711);
      header.setUrlPtrPos(12345);
      header.setTitleIdxPos(23456);
      header.setClusterCount(14);
      header.setClusterPtrPos(45678);
      header.setMainPage(11);
      header.setLayoutPage(13);

      CXXTOOLS_UNIT_ASSERT_EQUALS(header.getUuid(), "1234567890abcdef");
      CXXTOOLS_UNIT_ASSERT_EQUALS(header.getArticleCount(), 4711);
      CXXTOOLS_UNIT_ASSERT_EQUALS(header.getUrlPtrPos(), 12345);
      CXXTOOLS_UNIT_ASSERT_EQUALS(header.getTitleIdxPos(), 23456);
      CXXTOOLS_UNIT_ASSERT_EQUALS(header.getClusterCount(), 14);
      CXXTOOLS_UNIT_ASSERT_EQUALS(header.getClusterPtrPos(), 45678);
      CXXTOOLS_UNIT_ASSERT_EQUALS(header.getMainPage(), 11);
      CXXTOOLS_UNIT_ASSERT_EQUALS(header.getLayoutPage(), 13);

      std::stringstream s;
      s << header;

      zim::Fileheader header2;
      s >> header2;

      CXXTOOLS_UNIT_ASSERT_EQUALS(s.tellg(), s.tellp());

      CXXTOOLS_UNIT_ASSERT_EQUALS(header2.getUuid(), "1234567890abcdef");
      CXXTOOLS_UNIT_ASSERT_EQUALS(header2.getArticleCount(), 4711);
      CXXTOOLS_UNIT_ASSERT_EQUALS(header2.getUrlPtrPos(), 12345);
      CXXTOOLS_UNIT_ASSERT_EQUALS(header2.getTitleIdxPos(), 23456);
      CXXTOOLS_UNIT_ASSERT_EQUALS(header2.getClusterCount(), 14);
      CXXTOOLS_UNIT_ASSERT_EQUALS(header2.getClusterPtrPos(), 45678);
      CXXTOOLS_UNIT_ASSERT_EQUALS(header2.getMainPage(), 11);
      CXXTOOLS_UNIT_ASSERT_EQUALS(header2.getLayoutPage(), 13);

    }

};

cxxtools::unit::RegisterTest<FileheaderTest> register_FileheaderTest;
