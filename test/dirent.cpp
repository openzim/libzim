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

#ifdef _WIN32
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#endif

namespace
{

std::unique_ptr<zim::Buffer> write_to_buffer(zim::writer::Dirent& dirent)
{
#ifdef _WIN32
  wchar_t cbase[MAX_PATH];
  wchar_t ctmp[MAX_PATH];
  GetTempPathW(MAX_PATH-14, cbase);
  // This create a file for us, ensure it is unique.
  // So we need to delete it and create the directory using the same name.
  GetTempFileNameW(cbase, L"test_dirent", 0, ctmp);
  auto tmp_fd = _open(ctmp, _O_CREAT | _O_TEMPORARY | _O_SHORT_LIVED | _O_RDWR | _O_TRUNC);
#else
  char tmpl[] = "/tmp/test_dirent_XXXXXX";
  auto tmp_fd = mkstemp(tmpl);
#endif
  dirent.write(tmp_fd);
  auto size = lseek(tmp_fd, 0, SEEK_END);

  char* content = new char[size];
  lseek(tmp_fd, 0, SEEK_SET);
  read(tmp_fd, content, size);
  close(tmp_fd);
  unlink(tmpl);
  return std::unique_ptr<zim::Buffer>(
      new zim::MemoryBuffer<true>(content, zim::zsize_t(size)));
}

size_t writenDirentSize(const zim::writer::Dirent& dirent)
{
  char tmpl[] = "/tmp/test_dirent_XXXXXX";
  auto tmp_fd = mkstemp(tmpl);
  dirent.write(tmp_fd);
  auto size = lseek(tmp_fd, 0, SEEK_END);
  close(tmp_fd);
  unlink(tmpl);
  return size;
}


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

  auto buffer = write_to_buffer(dirent);
  zim::Dirent dirent2(std::move(buffer));

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

  auto buffer = write_to_buffer(dirent);
  zim::Dirent dirent2(std::move(buffer));

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

  auto buffer = write_to_buffer(dirent);
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

  auto buffer = write_to_buffer(dirent);
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

  auto buffer = write_to_buffer(dirent);
  zim::Dirent dirent2(std::move(buffer));

  ASSERT_TRUE(!dirent2.isRedirect());
  ASSERT_TRUE(!dirent2.isLinktarget());
  ASSERT_TRUE(dirent2.isDeleted());
  ASSERT_EQ(dirent2.getNamespace(), 'A');
  ASSERT_EQ(dirent2.getUrl(), "Bar");
  ASSERT_EQ(dirent2.getTitle(), "Bar");
}

TEST(DirentTest, dirent_size)
{
  zim::writer::Dirent dirent;
  std::string s;
  dirent.setArticle(17, zim::cluster_index_t(45), zim::blob_index_t(1234));
  dirent.setUrl(zim::writer::Url('A', "Bar"));

  // case url set, title empty, extralen empty
  ASSERT_EQ(dirent.getDirentSize(), writenDirentSize(dirent));

  // case url set, title set, extralen empty
  dirent.setTitle("Foo");
  ASSERT_EQ(dirent.getDirentSize(), writenDirentSize(dirent));

  // case url set, title empty
  dirent.setTitle(std::string());
  ASSERT_EQ(dirent.getDirentSize(), writenDirentSize(dirent));
}

TEST(DirentTest, redirect_dirent_size)
{
  zim::writer::Dirent targetDirent;
  targetDirent.setIdx(zim::article_index_t(321));
  zim::writer::Dirent dirent;
  dirent.setUrl(zim::writer::Url('A', "Bar"));
  dirent.setRedirect(&targetDirent);

  ASSERT_EQ(dirent.getDirentSize(), writenDirentSize(dirent));
}

}  // namespace
