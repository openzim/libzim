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

#include <cstring>
#include <iostream>
#include <sstream>

#include "gtest/gtest.h"

#include "../src/buffer.h"
#include "../src/dirent.h"

namespace
{
TEST(DirentTest, set_get_data_dirent)
{
  zim::Dirent dirent;
  dirent.setUrl('A', "Bar");
  dirent.setArticle(17, zim::cluster_index_t(45), zim::blob_index_t(1234));
  dirent.setVersion(54346);

  ASSERT_TRUE(!dirent.isRedirect());
  ASSERT_EQ(dirent.getNamespace(), 'A');
  ASSERT_EQ(dirent.getUrl(), "Bar");
  ASSERT_EQ(dirent.getTitle(), "Bar");
  ASSERT_EQ(dirent.getParameter(), "");
  ASSERT_EQ(dirent.getClusterNumber().v, 45U);
  ASSERT_EQ(dirent.getBlobNumber().v, 1234U);
  ASSERT_EQ(dirent.getVersion(), 54346U);

  dirent.setTitle("Foo");
  ASSERT_EQ(dirent.getNamespace(), 'A');
  ASSERT_EQ(dirent.getUrl(), "Bar");
  ASSERT_EQ(dirent.getTitle(), "Foo");
  ASSERT_EQ(dirent.getParameter(), "");
}

TEST(DirentTest, read_write_article_dirent)
{
  zim::Dirent dirent;
  dirent.setUrl('A', "Bar");
  dirent.setTitle("Foo");
  dirent.setArticle(17, zim::cluster_index_t(45), zim::blob_index_t(1234));
  dirent.setVersion(54346);

  ASSERT_TRUE(!dirent.isRedirect());
  ASSERT_EQ(dirent.getNamespace(), 'A');
  ASSERT_EQ(dirent.getUrl(), "Bar");
  ASSERT_EQ(dirent.getTitle(), "Foo");
  ASSERT_EQ(dirent.getParameter(), "");
  ASSERT_EQ(dirent.getClusterNumber().v, 45U);
  ASSERT_EQ(dirent.getBlobNumber().v, 1234U);
  ASSERT_EQ(dirent.getVersion(), 54346U);

  std::stringstream s;
  s << dirent;

  std::string str_content = s.str();
  int size = str_content.size();
  char* content = new char[size];
  memcpy(content, str_content.c_str(), size);
  auto buffer = std::unique_ptr<zim::Buffer>(
      new zim::MemoryBuffer<true>(content, zim::zsize_t(size)));
  zim::Dirent dirent2(std::move(buffer));

  ASSERT_TRUE(!s.fail());

  ASSERT_TRUE(!dirent2.isRedirect());
  ASSERT_EQ(dirent2.getNamespace(), 'A');
  ASSERT_EQ(dirent2.getTitle(), "Foo");
  ASSERT_EQ(dirent2.getParameter(), "");
  ASSERT_EQ(dirent2.getClusterNumber().v, 45U);
  ASSERT_EQ(dirent2.getBlobNumber().v, 1234U);
  ASSERT_EQ(dirent2.getVersion(), 54346U);
}

TEST(DirentTest, read_write_article_dirent_unicode)
{
  zim::Dirent dirent;
  dirent.setUrl('A', "L\xc3\xbcliang");
  dirent.setArticle(17, zim::cluster_index_t(45), zim::blob_index_t(1234));

  ASSERT_TRUE(!dirent.isRedirect());
  ASSERT_EQ(dirent.getNamespace(), 'A');
  ASSERT_EQ(dirent.getUrl(), "L\xc3\xbcliang");
  ASSERT_EQ(dirent.getTitle(), "L\xc3\xbcliang");
  ASSERT_EQ(dirent.getParameter(), "");
  ASSERT_EQ(dirent.getClusterNumber().v, 45U);
  ASSERT_EQ(dirent.getBlobNumber().v, 1234U);

  std::stringstream s;
  s << dirent;

  std::string str_content = s.str();
  int size = str_content.size();
  char* content = new char[size];
  memcpy(content, str_content.c_str(), size);
  auto buffer = std::unique_ptr<zim::Buffer>(
      new zim::MemoryBuffer<true>(content, zim::zsize_t(size)));
  zim::Dirent dirent2(std::move(buffer));

  ASSERT_TRUE(!s.fail());

  ASSERT_TRUE(!dirent2.isRedirect());
  ASSERT_EQ(dirent2.getNamespace(), 'A');
  ASSERT_EQ(dirent2.getUrl(), "L\xc3\xbcliang");
  ASSERT_EQ(dirent2.getTitle(), "L\xc3\xbcliang");
  ASSERT_EQ(dirent2.getParameter(), "");
  ASSERT_EQ(dirent2.getClusterNumber().v, 45U);
  ASSERT_EQ(dirent2.getBlobNumber().v, 1234U);
}

TEST(DirentTest, read_write_article_dirent_parameter)
{
  zim::Dirent dirent;
  dirent.setUrl('A', "Foo");
  dirent.setParameter("bar");
  dirent.setArticle(17, zim::cluster_index_t(45), zim::blob_index_t(1234));

  ASSERT_TRUE(!dirent.isRedirect());
  ASSERT_EQ(dirent.getNamespace(), 'A');
  ASSERT_EQ(dirent.getUrl(), "Foo");
  ASSERT_EQ(dirent.getTitle(), "Foo");
  ASSERT_EQ(dirent.getParameter(), "bar");
  ASSERT_EQ(dirent.getClusterNumber().v, 45U);
  ASSERT_EQ(dirent.getBlobNumber().v, 1234U);

  std::stringstream s;
  s << dirent;

  std::string str_content = s.str();
  int size = str_content.size();
  char* content = new char[size];
  memcpy(content, str_content.c_str(), size);
  auto buffer = std::unique_ptr<zim::Buffer>(
      new zim::MemoryBuffer<true>(content, zim::zsize_t(size)));
  zim::Dirent dirent2(std::move(buffer));

  ASSERT_TRUE(!s.fail());

  ASSERT_TRUE(!dirent2.isRedirect());
  ASSERT_EQ(dirent2.getNamespace(), 'A');
  ASSERT_EQ(dirent2.getTitle(), "Foo");
  ASSERT_EQ(dirent2.getParameter(), "bar");
  ASSERT_EQ(dirent2.getClusterNumber().v, 45U);
  ASSERT_EQ(dirent2.getBlobNumber().v, 1234U);
}

TEST(DirentTest, read_write_redirect_dirent)
{
  zim::Dirent dirent;
  dirent.setUrl('A', "Bar");
  dirent.setParameter("baz");
  dirent.setRedirect(zim::article_index_t(321));

  ASSERT_TRUE(dirent.isRedirect());
  ASSERT_EQ(dirent.getNamespace(), 'A');
  ASSERT_EQ(dirent.getUrl(), "Bar");
  ASSERT_EQ(dirent.getParameter(), "baz");
  ASSERT_EQ(dirent.getRedirectIndex().v, 321U);

  std::stringstream s;
  s << dirent;

  std::string str_content = s.str();
  int size = str_content.size();
  char* content = new char[size];
  memcpy(content, str_content.c_str(), size);
  auto buffer = std::unique_ptr<zim::Buffer>(
      new zim::MemoryBuffer<true>(content, zim::zsize_t(size)));
  zim::Dirent dirent2(std::move(buffer));

  ASSERT_TRUE(dirent2.isRedirect());
  ASSERT_EQ(dirent2.getNamespace(), 'A');
  ASSERT_EQ(dirent2.getUrl(), "Bar");
  ASSERT_EQ(dirent2.getTitle(), "Bar");
  ASSERT_EQ(dirent2.getParameter(), "baz");
  ASSERT_EQ(dirent2.getRedirectIndex().v, 321U);
}

TEST(DirentTest, read_write_linktarget_dirent)
{
  zim::Dirent dirent;
  dirent.setUrl('A', "Bar");
  dirent.setLinktarget();

  ASSERT_TRUE(!dirent.isRedirect());
  ASSERT_TRUE(dirent.isLinktarget());
  ASSERT_TRUE(!dirent.isDeleted());
  ASSERT_EQ(dirent.getNamespace(), 'A');
  ASSERT_EQ(dirent.getUrl(), "Bar");

  std::stringstream s;
  s << dirent;

  std::string str_content = s.str();
  int size = str_content.size();
  char* content = new char[size];
  memcpy(content, str_content.c_str(), size);
  auto buffer = std::unique_ptr<zim::Buffer>(
      new zim::MemoryBuffer<true>(content, zim::zsize_t(size)));
  zim::Dirent dirent2(std::move(buffer));

  ASSERT_TRUE(!dirent2.isRedirect());
  ASSERT_TRUE(dirent2.isLinktarget());
  ASSERT_TRUE(!dirent2.isDeleted());
  ASSERT_EQ(dirent2.getNamespace(), 'A');
  ASSERT_EQ(dirent2.getUrl(), "Bar");
  ASSERT_EQ(dirent2.getTitle(), "Bar");
}

TEST(DirentTest, read_write_deleted_dirent)
{
  zim::Dirent dirent;
  dirent.setUrl('A', "Bar");
  dirent.setDeleted();

  ASSERT_TRUE(!dirent.isRedirect());
  ASSERT_TRUE(!dirent.isLinktarget());
  ASSERT_TRUE(dirent.isDeleted());
  ASSERT_EQ(dirent.getNamespace(), 'A');
  ASSERT_EQ(dirent.getUrl(), "Bar");

  std::stringstream s;
  s << dirent;

  std::string str_content = s.str();
  int size = str_content.size();
  char* content = new char[size];
  memcpy(content, str_content.c_str(), size);
  auto buffer = std::unique_ptr<zim::Buffer>(
      new zim::MemoryBuffer<true>(content, zim::zsize_t(size)));
  zim::Dirent dirent2(std::move(buffer));

  ASSERT_TRUE(!dirent2.isRedirect());
  ASSERT_TRUE(!dirent2.isLinktarget());
  ASSERT_TRUE(dirent2.isDeleted());
  ASSERT_EQ(dirent2.getNamespace(), 'A');
  ASSERT_EQ(dirent2.getUrl(), "Bar");
  ASSERT_EQ(dirent2.getTitle(), "Bar");
}

std::string direntAsString(const zim::Dirent& dirent)
{
  std::ostringstream d;
  d << dirent;
  return d.str();
}

TEST(DirentTest, dirent_size)
{
  zim::Dirent dirent;
  std::string s;
  dirent.setArticle(17, zim::cluster_index_t(45), zim::blob_index_t(1234));
  dirent.setUrl('A', "Bar");

  // case url set, title empty, extralen empty
  s = direntAsString(dirent);
  ASSERT_EQ(dirent.getDirentSize(), s.size());

  // case url set, title set, extralen empty
  dirent.setTitle("Foo");
  s = direntAsString(dirent);
  ASSERT_EQ(dirent.getDirentSize(), s.size());

  // case url set, title set, extralen set
  dirent.setParameter("baz");
  s = direntAsString(dirent);
  ASSERT_EQ(dirent.getDirentSize(), s.size());

  // case url set, title empty, extralen set
  dirent.setTitle(std::string());
  s = direntAsString(dirent);
  ASSERT_EQ(dirent.getDirentSize(), s.size());
}

TEST(DirentTest, redirect_dirent_size)
{
  zim::Dirent dirent;
  dirent.setUrl('A', "Bar");
  dirent.setParameter("baz");
  dirent.setRedirect(zim::article_index_t(321));

  std::ostringstream d;
  d << dirent;

  ASSERT_EQ(dirent.getDirentSize(), d.str().size());
}

}  // namespace

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
