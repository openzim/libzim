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

#include <zim/dirent.h>
#include <iostream>
#include <sstream>

#include <cxxtools/unit/testsuite.h>
#include <cxxtools/unit/registertest.h>

class DirentTest : public cxxtools::unit::TestSuite
{
  public:
    DirentTest()
      : cxxtools::unit::TestSuite("zim::DirentTest")
    {
      registerMethod("SetGetDataDirent", *this, &DirentTest::SetGetDataDirent);
      registerMethod("ReadWriteArticleDirent", *this, &DirentTest::ReadWriteArticleDirent);
      registerMethod("ReadWriteArticleDirentU", *this, &DirentTest::ReadWriteArticleDirentU);
      registerMethod("ReadWriteArticleDirentP", *this, &DirentTest::ReadWriteArticleDirentP);
      registerMethod("ReadWriteRedirectDirent", *this, &DirentTest::ReadWriteRedirectDirent);
      registerMethod("DirentSize", *this, &DirentTest::DirentSize);
      registerMethod("RedirectDirentSize", *this, &DirentTest::RedirectDirentSize);
    }

    void SetGetDataDirent()
    {
      zim::Dirent dirent;
      dirent.setUrl('A', "Bar");
      dirent.setArticle(17, 45, 1234);
      dirent.setVersion(54346);

      CXXTOOLS_UNIT_ASSERT(!dirent.isRedirect());
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getNamespace(), 'A');
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getUrl(), "Bar");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getTitle(), "Bar");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getParameter(), "");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getClusterNumber(), 45);
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getBlobNumber(), 1234);
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getVersion(), 54346);

      dirent.setTitle("Foo");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getNamespace(), 'A');
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getUrl(), "Bar");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getTitle(), "Foo");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getParameter(), "");
    }

    void ReadWriteArticleDirent()
    {
      zim::Dirent dirent;
      dirent.setUrl('A', "Bar");
      dirent.setTitle("Foo");
      dirent.setArticle(17, 45, 1234);
      dirent.setVersion(54346);

      CXXTOOLS_UNIT_ASSERT(!dirent.isRedirect());
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getNamespace(), 'A');
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getUrl(), "Bar");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getTitle(), "Foo");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getParameter(), "");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getClusterNumber(), 45);
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getBlobNumber(), 1234);
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getVersion(), 54346);

      std::stringstream s;
      s << dirent;

      zim::Dirent dirent2;
      s >> dirent2;

      CXXTOOLS_UNIT_ASSERT(!s.fail());

      CXXTOOLS_UNIT_ASSERT_EQUALS(s.tellg(), s.tellp());

      CXXTOOLS_UNIT_ASSERT(!dirent2.isRedirect());
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getNamespace(), 'A');
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getTitle(), "Foo");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getParameter(), "");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getClusterNumber(), 45);
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getBlobNumber(), 1234);
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getVersion(), 54346);

    }

    void ReadWriteArticleDirentU()
    {
      zim::Dirent dirent;
      dirent.setUrl('A', "L\xc3\xbcliang");
      dirent.setArticle(17, 45, 1234);

      CXXTOOLS_UNIT_ASSERT(!dirent.isRedirect());
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getNamespace(), 'A');
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getUrl(), "L\xc3\xbcliang");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getTitle(), "L\xc3\xbcliang");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getParameter(), "");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getClusterNumber(), 45);
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getBlobNumber(), 1234);

      std::stringstream s;
      s << dirent;

      zim::Dirent dirent2;
      s >> dirent2;

      CXXTOOLS_UNIT_ASSERT(!s.fail());

      CXXTOOLS_UNIT_ASSERT_EQUALS(s.tellg(), s.tellp());

      CXXTOOLS_UNIT_ASSERT(!dirent2.isRedirect());
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getNamespace(), 'A');
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getUrl(), "L\xc3\xbcliang");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getTitle(), "L\xc3\xbcliang");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getParameter(), "");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getClusterNumber(), 45);
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getBlobNumber(), 1234);

    }

    void ReadWriteArticleDirentP()
    {
      zim::Dirent dirent;
      dirent.setUrl('A', "Foo");
      dirent.setParameter("bar");
      dirent.setArticle(17, 45, 1234);

      CXXTOOLS_UNIT_ASSERT(!dirent.isRedirect());
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getNamespace(), 'A');
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getUrl(), "Foo");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getTitle(), "Foo");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getParameter(), "bar");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getClusterNumber(), 45);
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getBlobNumber(), 1234);

      std::stringstream s;
      s << dirent;

      zim::Dirent dirent2;
      s >> dirent2;

      CXXTOOLS_UNIT_ASSERT(!s.fail());

      CXXTOOLS_UNIT_ASSERT_EQUALS(s.tellg(), s.tellp());

      CXXTOOLS_UNIT_ASSERT(!dirent2.isRedirect());
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getNamespace(), 'A');
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getTitle(), "Foo");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getParameter(), "bar");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getClusterNumber(), 45);
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getBlobNumber(), 1234);
    }

    void ReadWriteRedirectDirent()
    {
      zim::Dirent dirent;
      dirent.setUrl('A', "Bar");
      dirent.setParameter("baz");
      dirent.setRedirect(321);

      CXXTOOLS_UNIT_ASSERT(dirent.isRedirect());
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getNamespace(), 'A');
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getUrl(), "Bar");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getParameter(), "baz");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getRedirectIndex(), 321);

      std::stringstream s;
      s << dirent;
      zim::Dirent dirent2;
      s >> dirent2;

      CXXTOOLS_UNIT_ASSERT_EQUALS(s.tellg(), s.tellp());

      CXXTOOLS_UNIT_ASSERT(dirent2.isRedirect());
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getNamespace(), 'A');
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getUrl(), "Bar");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getTitle(), "Bar");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getParameter(), "baz");
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent2.getRedirectIndex(), 321);
    }

    std::string direntAsString(const zim::Dirent& dirent)
    {
      std::ostringstream d;
      d << dirent;
      return d.str();
    }

    void DirentSize()
    {
      zim::Dirent dirent;
      std::string s;
      dirent.setArticle(17, 45, 1234);
      dirent.setUrl('A', "Bar");

      // case url set, title empty, extralen empty
      s = direntAsString(dirent);
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getDirentSize(), s.size());

      // case url set, title set, extralen empty
      dirent.setTitle("Foo");
      s = direntAsString(dirent);
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getDirentSize(), s.size());

      // case url set, title set, extralen set
      dirent.setParameter("baz");
      s = direntAsString(dirent);
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getDirentSize(), s.size());

      // case url set, title empty, extralen set
      dirent.setTitle(std::string());
      s = direntAsString(dirent);
      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getDirentSize(), s.size());
    }

    void RedirectDirentSize()
    {
      zim::Dirent dirent;
      dirent.setUrl('A', "Bar");
      dirent.setParameter("baz");
      dirent.setRedirect(321);

      std::ostringstream d;
      d << dirent;

      CXXTOOLS_UNIT_ASSERT_EQUALS(dirent.getDirentSize(), d.str().size());
    }

};

cxxtools::unit::RegisterTest<DirentTest> register_DirentTest;
