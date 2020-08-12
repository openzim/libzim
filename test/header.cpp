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

#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <fileapi.h>
#endif

#include <iostream>
#include <sstream>

#include "gtest/gtest.h"

#include <zim/fileheader.h>

#include "../src/buffer.h"

#include "tempfile.h"

namespace
{

using zim::unittests::TempFile;

std::shared_ptr<zim::Buffer> write_to_buffer(zim::Fileheader &header)
{
  const TempFile tmpFile("test_header");
  const auto tmp_fd = tmpFile.fd();
  header.write(tmp_fd);
  auto size = lseek(tmp_fd, 0, SEEK_END);

  auto buf = std::make_shared<zim::MemoryBuffer>(zim::zsize_t(size));
  lseek(tmp_fd, 0, SEEK_SET);
  if (read(tmp_fd, buf->buf(), size) == -1)
    throw std::runtime_error("Cannot read");
  return buf;
}

TEST(HeaderTest, read_write_header)
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
  header.setMimeListPos(72);

  ASSERT_EQ(header.getUuid(), "1234567890abcdef");
  ASSERT_EQ(header.getArticleCount(), 4711U);
  ASSERT_EQ(header.getUrlPtrPos(), 12345U);
  ASSERT_EQ(header.getTitleIdxPos(), 23456U);
  ASSERT_EQ(header.getClusterCount(), 14U);
  ASSERT_EQ(header.getClusterPtrPos(), 45678U);
  ASSERT_EQ(header.getMainPage(), 11U);
  ASSERT_EQ(header.getLayoutPage(), 13U);
  ASSERT_EQ(header.getMimeListPos(), 72U);

  auto buffer = write_to_buffer(header);
  zim::Fileheader header2;
  header2.read(buffer);

  ASSERT_EQ(header2.getUuid(), "1234567890abcdef");
  ASSERT_EQ(header2.getArticleCount(), 4711U);
  ASSERT_EQ(header2.getUrlPtrPos(), 12345U);
  ASSERT_EQ(header2.getTitleIdxPos(), 23456U);
  ASSERT_EQ(header2.getClusterCount(), 14U);
  ASSERT_EQ(header2.getClusterPtrPos(), 45678U);
  ASSERT_EQ(header2.getMainPage(), 11U);
  ASSERT_EQ(header2.getLayoutPage(), 13U);
}

}  // namespace
