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
#include "../src/dirent_accessor.h"
#include "../src/_dirent.h"
#include "../src/fileheader.h"
#include "../src/cluster.h"
#include "../src/rawstreamreader.h"

#include "gtest/gtest.h"

namespace
{

using namespace zim;

struct NoneType {};
const NoneType None;

template<typename T>
struct Optional{
  Optional(NoneType none) : active(false) {};
  Optional(T value) : active(true), value(value) {};
  void check(const T& value) { if (active) ASSERT_EQ(this->value, value); }
  bool active;
  T    value;
};

template<>
struct Optional<const std::string> {
  Optional(NoneType none) : active(false) {};
  Optional(std::string value) : active(true), value(value) {};
  Optional(const char* value) : active(true), value(value) {};
  void check(const std::string& value) { if (active) ASSERT_EQ(this->value, value); }
  bool active;
  std::string value;
};

void test_article_dirent(
  std::shared_ptr<const Dirent> dirent,
  Optional<char> ns,
  Optional<const std::string> url,
  Optional<const std::string> title,
  Optional<uint16_t> mimetype,
  Optional<cluster_index_t> clusterNumber,
  Optional<blob_index_t> blobNumber)
{
  ASSERT_TRUE(dirent->isArticle());
  ns.check(dirent->getNamespace());
  url.check(dirent->getUrl());
  title.check(dirent->getTitle());
  mimetype.check(dirent->getMimeType());
  clusterNumber.check(dirent->getClusterNumber());
  blobNumber.check(dirent->getBlobNumber());
}

void test_redirect_dirent(
  std::shared_ptr<const Dirent> dirent,
  Optional<char> ns,
  Optional<const std::string> url,
  Optional<const std::string> title,
  Optional<entry_index_t> target)
{
  ASSERT_TRUE(dirent->isRedirect());
  ns.check(dirent->getNamespace());
  url.check(dirent->getUrl());
  title.check(dirent->getTitle());
  target.check(dirent->getRedirectIndex());
}

TEST(ZimCreator, createEmptyZim)
{
  unittests::TempFile temp("emptyzimfile");
  auto tempPath = temp.path() + ".zim";
  writer::Creator creator;
  creator.startZimCreation(tempPath);
  creator.finishZimCreation();

  // Do not use the high level Archive to test that zim file is correctly created but lower structure.
  auto fileCompound = std::make_shared<FileCompound>(tempPath);
  auto reader = std::make_shared<MultiPartFileReader>(fileCompound);
  Fileheader header;
  header.read(*reader);
  ASSERT_FALSE(header.hasMainPage());
#if defined(ENABLE_XAPIAN)
  entry_index_type nb_entry = 3; // xapiantitleIndex and titleListIndexes (*2)
  int xapian_mimetype = 0;
  int listing_mimetype = 1;
#else
  entry_index_type nb_entry = 2; // titleListIndexes (*2)
  int listing_mimetype = 0;
#endif
  ASSERT_EQ(header.getArticleCount(), nb_entry);

  //Read the only one item existing.
  auto urlPtrReader = reader->sub_reader(offset_t(header.getUrlPtrPos()), zsize_t(sizeof(offset_t)*header.getArticleCount()));
  DirectDirentAccessor direntAccessor(std::make_shared<DirentReader>(reader), std::move(urlPtrReader), entry_index_t(header.getArticleCount()));
  std::shared_ptr<const Dirent> dirent;

  dirent = direntAccessor.getDirent(entry_index_t(0));
  test_article_dirent(dirent, 'X', "listing/titleOrdered/v0", None, listing_mimetype, cluster_index_t(0), None);
  auto v0BlobIndex = dirent->getBlobNumber();

  dirent = direntAccessor.getDirent(entry_index_t(1));
  test_article_dirent(dirent, 'X', "listing/titleOrdered/v1", None, listing_mimetype, cluster_index_t(0), None);
  auto v1BlobIndex = dirent->getBlobNumber();

#if defined(ENABLE_XAPIAN)
  dirent = direntAccessor.getDirent(entry_index_t(2));
  test_article_dirent(dirent, 'X', "title/xapian", None, xapian_mimetype, cluster_index_t(0), None);
#endif

  auto clusterPtrPos = header.getClusterPtrPos();
  auto clusterOffset = offset_t(reader->read_uint<offset_type>(offset_t(clusterPtrPos)));
  auto cluster = Cluster::read(*reader, clusterOffset);
  ASSERT_EQ(cluster->getCompression(), CompressionType::zimcompNone);
  ASSERT_EQ(cluster->count(), blob_index_t(nb_entry));
  auto blob = cluster->getBlob(v0BlobIndex);
  ASSERT_EQ(blob.size(), nb_entry*sizeof(title_index_t));
  blob = cluster->getBlob(v1BlobIndex);
  ASSERT_EQ(blob.size(), 0);
}


class TestItem : public writer::Item
{
  public:
    TestItem(const std::string& path, const std::string& title, const std::string& content):
     path(path), title(title), content(content)  { }
    virtual ~TestItem() = default;

    virtual std::string getPath() const { return path; };
    virtual std::string getTitle() const { return title; };
    virtual std::string getMimeType() const { return "text/html"; };
    virtual writer::Hints getHints() const { return { { writer::FRONT_ARTICLE, 1 } }; }

    virtual std::unique_ptr<writer::ContentProvider> getContentProvider() const {
      return std::unique_ptr<writer::ContentProvider>(new writer::StringProvider(content));
    }

  std::string path;
  std::string title;
  std::string content;
};

TEST(ZimCreator, createZim)
{
  unittests::TempFile temp("zimfile");
  auto tempPath = temp.path() + ".zim";
  writer::Creator creator;
  creator.configIndexing(true, "eng");
  creator.startZimCreation(tempPath);
  auto item = std::make_shared<TestItem>("foo", "Foo", "FooContent");
  creator.addItem(item);
  // Be sure that title order is not the same that url order
  item = std::make_shared<TestItem>("foo2", "AFoo", "Foo2Content");
  creator.addItem(item);
  creator.addMetadata("Title", "This is a title");
  creator.setMainPath("foo");
  creator.addRedirection("foo3", "FooRedirection", "foo"); // No a front article.
  creator.addRedirection("foo4", "FooRedirection", "NoExistant"); // Invalid redirection, must be removed by creator
  creator.finishZimCreation();

  // Do not use the high level Archive to test that zim file is correctly created but lower structure.
  auto fileCompound = std::make_shared<FileCompound>(tempPath);
  auto reader = std::make_shared<MultiPartFileReader>(fileCompound);
  Fileheader header;
  header.read(*reader);
  ASSERT_TRUE(header.hasMainPage());
#if defined(ENABLE_XAPIAN)
  entry_index_type nb_entry = 9; // xapiantitleIndex + xapianfulltextIndex + foo + foo2 + foo3 + Title + mainPage + titleListIndexes*2
  int xapian_mimetype = 0;
  int listing_mimetype = 1;
  int html_mimetype = 2;
  int plain_mimetype = 3;
#else
  entry_index_type nb_entry = 7; // foo + foo2 + foo3 + Title + mainPage + titleListIndexes*2
  int listing_mimetype = 0;
  int html_mimetype = 1;
  int plain_mimetype = 2;
#endif

  ASSERT_EQ(header.getArticleCount(), nb_entry);

  // Read dirent
  auto urlPtrReader = reader->sub_reader(offset_t(header.getUrlPtrPos()), zsize_t(sizeof(offset_t)*header.getArticleCount()));
  DirectDirentAccessor direntAccessor(std::make_shared<DirentReader>(reader), std::move(urlPtrReader), entry_index_t(header.getArticleCount()));
  std::shared_ptr<const Dirent> dirent;

  entry_index_type direntIdx = 0;
  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_redirect_dirent(dirent, '-', "mainPage", "mainPage", entry_index_t(1));

  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_article_dirent(dirent, 'C', "foo", "Foo", html_mimetype, cluster_index_t(0), None);
  auto fooBlobIndex = dirent->getBlobNumber();

  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_article_dirent(dirent, 'C', "foo2", "AFoo", html_mimetype, cluster_index_t(0), None);
  auto foo2BlobIndex = dirent->getBlobNumber();

  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_redirect_dirent(dirent, 'C', "foo3", "FooRedirection", entry_index_t(1));

  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_article_dirent(dirent, 'M', "Title", "Title", plain_mimetype, cluster_index_t(0), None);
  auto metaBlobIndex = dirent->getBlobNumber();

#if defined(ENABLE_XAPIAN)
  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_article_dirent(dirent, 'X', "fulltext/xapian", "fulltext/xapian", xapian_mimetype, cluster_index_t(1), None);
#endif

  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_article_dirent(dirent, 'X', "listing/titleOrdered/v0", None, listing_mimetype, cluster_index_t(1), None);
  auto v0BlobIndex = dirent->getBlobNumber();

  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_article_dirent(dirent, 'X', "listing/titleOrdered/v1", None, listing_mimetype, cluster_index_t(1), None);
  auto v1BlobIndex = dirent->getBlobNumber();

#if defined(ENABLE_XAPIAN)
  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_article_dirent(dirent, 'X', "title/xapian", "title/xapian", xapian_mimetype, cluster_index_t(1), None);
#endif

  auto clusterPtrPos = header.getClusterPtrPos();

  // Test main content
  auto clusterOffset = offset_t(reader->read_uint<offset_type>(offset_t(clusterPtrPos)));
  auto cluster = Cluster::read(*reader, clusterOffset);
  ASSERT_EQ(cluster->getCompression(), CompressionType::zimcompZstd);
  ASSERT_EQ(cluster->count(), blob_index_t(3));

  auto blob = cluster->getBlob(fooBlobIndex);
  ASSERT_EQ(std::string(blob), "FooContent");

  blob = cluster->getBlob(foo2BlobIndex);
  ASSERT_EQ(std::string(blob), "Foo2Content");

  blob = cluster->getBlob(metaBlobIndex);
  ASSERT_EQ(std::string(blob), "This is a title");


  // Test listing content
  clusterOffset = offset_t(reader->read_uint<offset_type>(offset_t(clusterPtrPos + 8)));
  cluster = Cluster::read(*reader, clusterOffset);
  ASSERT_EQ(cluster->getCompression(), CompressionType::zimcompNone);
  ASSERT_EQ(cluster->count(), blob_index_t(nb_entry-5)); // 5 entries are not content entries

  blob = cluster->getBlob(v0BlobIndex);
  ASSERT_EQ(blob.size(), nb_entry*sizeof(title_index_t));
  std::vector<char> blob0Data(blob.data(), blob.end());
  std::vector<char> expectedBlob0Data = {
    0, 0, 0, 0,
    2, 0, 0, 0,
    1, 0, 0, 0,
    3, 0, 0, 0,
    4, 0, 0, 0,
    5, 0, 0, 0,
    6, 0, 0, 0
#if defined(ENABLE_XAPIAN)
    ,7, 0, 0, 0
    ,8, 0, 0, 0
#endif
    };
  ASSERT_EQ(blob0Data, expectedBlob0Data);

  blob = cluster->getBlob(v1BlobIndex);
  ASSERT_EQ(blob.size(), 2*sizeof(title_index_t));
  std::vector<char> blob1Data(blob.data(), blob.end());
  std::vector<char> expectedBlob1Data = {
    2, 0, 0, 0,
    1, 0, 0, 0
  };
  ASSERT_EQ(blob1Data, expectedBlob1Data);
}


} // unnamed namespace
