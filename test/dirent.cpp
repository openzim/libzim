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
#include "../src/_dirent.h"
#include "../src/writer/_dirent.h"

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
  zim::writer::Dirent dirent;
  dirent.setUrl(zim::writer::Url('A', "Bar"));
  dirent.setTitle("Foo");
  dirent.setArticle(17, zim::cluster_index_t(45), zim::blob_index_t(1234));

  ASSERT_TRUE(!dirent.isRedirect());
  ASSERT_EQ(dirent.getNamespace(), 'A');
  ASSERT_EQ(dirent.getUrl(), "Bar");
  ASSERT_EQ(dirent.getTitle(), "Foo");
  ASSERT_EQ(dirent.getClusterNumber().v, 45U);
  ASSERT_EQ(dirent.getBlobNumber().v, 1234U);
  ASSERT_EQ(dirent.getVersion(), 0U);

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
  ASSERT_EQ(dirent2.getVersion(), 0U);
}

TEST(DirentTest, read_write_article_dirent_unicode)
{
  zim::writer::Dirent dirent;
  dirent.setUrl(zim::writer::Url('A', "L\xc3\xbcliang"));
  dirent.setArticle(17, zim::cluster_index_t(45), zim::blob_index_t(1234));

  ASSERT_TRUE(!dirent.isRedirect());
  ASSERT_EQ(dirent.getNamespace(), 'A');
  ASSERT_EQ(dirent.getUrl(), "L\xc3\xbcliang");
  ASSERT_EQ(dirent.getTitle(), "L\xc3\xbcliang");
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

TEST(DirentTest, read_write_redirect_dirent)
{
  zim::writer::Dirent targetDirent;
  targetDirent.setIdx(zim::article_index_t(321));
  zim::writer::Dirent dirent;
  dirent.setUrl(zim::writer::Url('A', "Bar"));
  dirent.setRedirect(&targetDirent);

  ASSERT_TRUE(dirent.isRedirect());
  ASSERT_EQ(dirent.getNamespace(), 'A');
  ASSERT_EQ(dirent.getUrl(), "Bar");
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
  ASSERT_EQ(dirent2.getRedirectIndex().v, 321U);
}

TEST(DirentTest, read_write_linktarget_dirent)
{
  zim::writer::Dirent dirent;
  dirent.setUrl(zim::writer::Url('A', "Bar"));
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
  zim::writer::Dirent dirent;
  dirent.setUrl(zim::writer::Url('A', "Bar"));
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

std::string direntAsString(const zim::writer::Dirent& dirent)
{
  std::ostringstream d;
  d << dirent;
  return d.str();
}

TEST(DirentTest, dirent_size)
{
  zim::writer::Dirent dirent;
  std::string s;
  dirent.setArticle(17, zim::cluster_index_t(45), zim::blob_index_t(1234));
  dirent.setUrl(zim::writer::Url('A', "Bar"));

  // case url set, title empty, extralen empty
  s = direntAsString(dirent);
  ASSERT_EQ(dirent.getDirentSize(), s.size());

  // case url set, title set, extralen empty
  dirent.setTitle("Foo");
  s = direntAsString(dirent);
  ASSERT_EQ(dirent.getDirentSize(), s.size());

  // case url set, title empty
  dirent.setTitle(std::string());
  s = direntAsString(dirent);
  ASSERT_EQ(dirent.getDirentSize(), s.size());
}

TEST(DirentTest, redirect_dirent_size)
{
  zim::writer::Dirent targetDirent;
  targetDirent.setIdx(zim::article_index_t(321));
  zim::writer::Dirent dirent;
  dirent.setUrl(zim::writer::Url('A', "Bar"));
  dirent.setRedirect(&targetDirent);

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
