/*
 * Copyright (C) 2021 Matthieu Gautier <mgautier@kymeria.fr>
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
#include <zim/writer/creator.h>
#include <zim/writer/item.h>
#include <zim/writer/contentProvider.h>

#include "tools.h"
#include "../src/file_compound.h"
#include "../src/file_reader.h"
#include "../src/direntreader.h"
#include "../src/fileheader.h"
#include "../src/cluster.h"
#include "../src/rawstreamreader.h"

#include "gtest/gtest.h"

namespace
{

using namespace zim;

template<typename T>
struct Filter{
  Filter() : active(false) {};
  Filter(T value) : active(true), value(value) {};
  bool active;
  T    value;
};

template<>
struct Filter<const std::string> {
  Filter() : active(false) {};
  Filter(std::string value) : active(true), value(value) {};
  Filter(const char* value) : active(true), value(value) {};
  bool active;
  std::string value;
};

void test_article_dirent(
  std::shared_ptr<const Dirent> dirent,
  Filter<char> ns,
  Filter<const std::string> url,
  Filter<const std::string> title,
  Filter<uint16_t> mimetype,
  Filter<cluster_index_t> clusterNumber,
  Filter<blob_index_t> blobNumber)
{
  ASSERT_TRUE(dirent->isArticle());
  if (ns.active) ASSERT_EQ(dirent->getNamespace(), ns.value);
  if (url.active) ASSERT_EQ(dirent->getUrl(), url.value);
  if (title.active) ASSERT_EQ(dirent->getTitle(), title.value);
  if (mimetype.active) ASSERT_EQ(dirent->getMimeType(), mimetype.value);
  if (clusterNumber.active) ASSERT_EQ(dirent->getClusterNumber(), clusterNumber.value);
  if (blobNumber.active) ASSERT_EQ(dirent->getBlobNumber(), blobNumber.value);
}

void test_redirect_dirent(
  std::shared_ptr<const Dirent> dirent,
  Filter<char> ns,
  Filter<const std::string> url,
  Filter<const std::string> title,
  Filter<entry_index_t> target)
{
  ASSERT_TRUE(dirent->isRedirect());
  if (ns.active) ASSERT_EQ(dirent->getNamespace(), ns.value);
  if (url.active) ASSERT_EQ(dirent->getUrl(), url.value);
  if (title.active) ASSERT_EQ(dirent->getTitle(), title.value);
  if (target.active) ASSERT_EQ(dirent->getRedirectIndex(), target.value);
}

TEST(ZimCreator, createEmptyZim)
{
  unittests::TempFile temp("emptyzimfile");
  auto tempPath = temp.path() + ".zim";
  writer::Creator creator;
  creator.startZimCreation(tempPath);
  creator.finishZimCreation();

  // Do not use the high level Archive to test that zim file is correctly created but lower structure.
  auto fd = DEFAULTFS::openFile(tempPath);
  auto size = fd.getSize();
  auto reader = std::make_shared<FileReader>(std::make_shared<typename DEFAULTFS::FD>(fd.release()), offset_t(0), size);
  Fileheader header;
  header.read(*reader);
  ASSERT_FALSE(header.hasMainPage());
#if defined(ENABLE_XAPIAN)
  ASSERT_EQ(header.getArticleCount(), 1); // xapiantitleIndex

  //Read the only one item existing.
  auto urlPtrPos = offset_t(header.getUrlPtrPos());
  auto direntOffset = offset_t(reader->read_uint<offset_type>(urlPtrPos));

  DirentReader direntReader(reader);
  auto dirent = direntReader.readDirent(direntOffset);
  test_article_dirent(dirent, 'X', "title/xapian", "title/xapian", 0, cluster_index_t(0), blob_index_t(0));
#else
  ASSERT_EQ(header.getArticleCount(), 0);
#endif
}


class TestItem : public writer::Item
{
  public:
    TestItem()  { }
    virtual ~TestItem() = default;

    virtual std::string getPath() const { return "foo"; };
    virtual std::string getTitle() const { return "Foo"; };
    virtual std::string getMimeType() const { return "text/html"; };

    virtual std::unique_ptr<writer::ContentProvider> getContentProvider() const {
      return std::unique_ptr<writer::ContentProvider>(new writer::StringProvider("FooContent"));
    }
};

TEST(ZimCreator, createZim)
{
  unittests::TempFile temp("zimfile");
  auto tempPath = temp.path() + ".zim";
  writer::Creator creator;
  creator.startZimCreation(tempPath);
  auto item = std::make_shared<TestItem>();
  creator.addItem(item);
  creator.addMetadata("Title", "This is a title");
  creator.setMainPath("foo");
  creator.finishZimCreation();

  // Do not use the high level Archive to test that zim file is correctly created but lower structure.
  auto fd = DEFAULTFS::openFile(tempPath);
  auto size = fd.getSize();
  auto reader = std::make_shared<FileReader>(std::make_shared<typename DEFAULTFS::FD>(fd.release()), offset_t(0), size);
  Fileheader header;
  header.read(*reader);
  ASSERT_TRUE(header.hasMainPage());
#if defined(ENABLE_XAPIAN)
  ASSERT_EQ(header.getArticleCount(), 4); //xapiantitleIndex + foo + Title + mainPage
  int xapian_mimetype = 0;
  int html_mimetype = 1;
  int plain_mimetype = 2;
#else
  ASSERT_EQ(header.getArticleCount(), 3); // foo + Title + mainPage
  int html_mimetype = 0;
  int plain_mimetype = 1;
#endif


  // Read dirent
  DirentReader direntReader(reader);
  auto urlPtrPos = header.getUrlPtrPos();
  offset_t direntOffset;
  std::shared_ptr<const Dirent> dirent;

  direntOffset = offset_t(reader->read_uint<offset_type>(offset_t(urlPtrPos)));
  dirent = direntReader.readDirent(direntOffset);
  test_redirect_dirent(dirent, '-', "mainPage", "mainPage", entry_index_t(1));

  direntOffset = offset_t(reader->read_uint<offset_type>(offset_t(urlPtrPos + 8)));
  dirent = direntReader.readDirent(direntOffset);
  test_article_dirent(dirent, 'C', "foo", "Foo", html_mimetype, cluster_index_t(0), blob_index_t(0));

  direntOffset = offset_t(reader->read_uint<offset_type>(offset_t(urlPtrPos + 16)));
  dirent = direntReader.readDirent(direntOffset);
  test_article_dirent(dirent, 'M', "Title", "Title", plain_mimetype, cluster_index_t(0), blob_index_t(1));

#if defined(ENABLE_XAPIAN)
  direntOffset = offset_t(reader->read_uint<offset_type>(offset_t(urlPtrPos + 24)));
  dirent = direntReader.readDirent(direntOffset);
  test_article_dirent(dirent, 'X', "title/xapian", "title/xapian", xapian_mimetype, cluster_index_t(1), blob_index_t(0));
#endif

  auto clusterPtrPos = header.getClusterPtrPos();
  auto clusterOffset = offset_t(reader->read_uint<offset_type>(offset_t(clusterPtrPos)));
  auto cluster = Cluster::read(*reader, clusterOffset);
  ASSERT_EQ(cluster->getCompression(), CompressionType::zimcompZstd);
  ASSERT_EQ(cluster->count(), blob_index_t(2));
  auto blob = cluster->getBlob(blob_index_t(0));
  ASSERT_EQ(std::string(blob.data(), blob.size()), "FooContent");
  blob = cluster->getBlob(blob_index_t(1));
  ASSERT_EQ(std::string(blob.data(), blob.size()), "This is a title");
}


} // unnamed namespace
