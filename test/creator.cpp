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
#include <zim/archive.h>
#include <zim/error.h>
#include <zim/tools.h>

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
  void check(const T& value) { if (active) { ASSERT_EQ(this->value, value); } }
  bool active;
  T    value;
};

template<>
struct Optional<const std::string> {
  Optional(NoneType none) : active(false) {};
  Optional(std::string value) : active(true), value(value) {};
  Optional(const char* value) : active(true), value(value) {};
  void check(const std::string& value) { if (active) { ASSERT_EQ(this->value, value); } }
  bool active;
  std::string value;
};

void test_article_dirent(
  std::shared_ptr<const Dirent> dirent,
  Optional<char> ns,
  Optional<const std::string> path,
  Optional<const std::string> title,
  Optional<uint16_t> mimetype,
  Optional<cluster_index_t> clusterNumber,
  Optional<blob_index_t> blobNumber)
{
  ASSERT_TRUE(dirent->isArticle());
  ns.check(dirent->getNamespace());
  path.check(dirent->getPath());
  title.check(dirent->getTitle());
  mimetype.check(dirent->getMimeType());
  clusterNumber.check(dirent->getClusterNumber());
  blobNumber.check(dirent->getBlobNumber());
}

void test_redirect_dirent(
  std::shared_ptr<const Dirent> dirent,
  Optional<char> ns,
  Optional<const std::string> path,
  Optional<const std::string> title,
  Optional<entry_index_t> target)
{
  ASSERT_TRUE(dirent->isRedirect());
  ns.check(dirent->getNamespace());
  path.check(dirent->getPath());
  title.check(dirent->getTitle());
  target.check(dirent->getRedirectIndex());
}

TEST(ZimCreator, DoNothing)
{
  // Creating a creator instance and do nothing on it should not crash.
  writer::Creator creator;
}

TEST(ZimCreator, createEmptyZim)
{
  unittests::TempFile temp("emptyzimfile");
  auto tempPath = temp.path();
  zim::Uuid uuid;
  // Force special char in the uuid to be sure they are not handled particularly.
  uuid.data[5] = '\n';
  uuid.data[10] = '\0';

  writer::Creator creator;
  creator.setUuid(uuid);
  creator.startZimCreation(tempPath);
  creator.finishZimCreation();

  // Do not use the high level Archive to test that zim file is correctly created but lower structure.
  auto fileCompound = std::make_shared<FileCompound>(tempPath);
  auto reader = std::make_shared<MultiPartFileReader>(fileCompound);
  Fileheader header;
  header.read(*reader);
  ASSERT_FALSE(header.hasMainPage());
  ASSERT_EQ(header.getArticleCount(), 2u); // counter + titleListIndexesv0

  //Read the only one item existing.
  auto pathPtrReader = reader->sub_reader(offset_t(header.getPathPtrPos()), zsize_t(sizeof(offset_t)*header.getArticleCount()));
  DirectDirentAccessor direntAccessor(std::make_shared<DirentReader>(reader), std::move(pathPtrReader), entry_index_t(header.getArticleCount()));
  std::shared_ptr<const Dirent> dirent;

  dirent = direntAccessor.getDirent(entry_index_t(0));
  test_article_dirent(dirent, 'M', "Counter", None, 1, cluster_index_t(0), None);

  dirent = direntAccessor.getDirent(entry_index_t(1));
  test_article_dirent(dirent, 'X', "listing/titleOrdered/v1", None, 0, cluster_index_t(1), None);
  auto v0BlobIndex = dirent->getBlobNumber();

  auto clusterPtrPos = header.getClusterPtrPos();
  auto clusterOffset = offset_t(reader->read_uint<offset_type>(offset_t(clusterPtrPos+8)));
  auto cluster = Cluster::read(*reader, clusterOffset);
  ASSERT_EQ(cluster->getCompression(), Cluster::Compression::None);
  ASSERT_EQ(cluster->count(), blob_index_t(1)); // Only titleListIndexesv0
  auto blob = cluster->getBlob(v0BlobIndex);
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
  auto tempPath = temp.path();
  zim::Uuid uuid;
  // Force special char in the uuid to be sure they are not handled particularly.
  uuid.data[5] = '\n';
  uuid.data[10] = '\0';

  writer::Creator creator;
  creator.setUuid(uuid);
  creator.configIndexing(true, "eng");
  creator.startZimCreation(tempPath);
  creator.addRedirection("foo", "WrongRedirection", "foobar", {{zim::writer::FRONT_ARTICLE, true}}); // Will be replaced by item
  auto item = std::make_shared<TestItem>("foo", "Foo", "FooContent");
  EXPECT_NO_THROW(creator.addItem(item));
  EXPECT_THROW(creator.addItem(item), std::runtime_error);
  // Be sure that title order is not the same that path order
  item = std::make_shared<TestItem>("foo2", "AFoo", "Foo2Content");
  creator.addItem(item);
  creator.addAlias("foo_bis", "The same Foo", "foo2");
  creator.addMetadata("Title", "This is a title");
  creator.addIllustration(48, "PNGBinaryContent48");
  creator.addIllustration(96, "PNGBinaryContent96");
  creator.setMainPath("foo");
  creator.addRedirection("foo3", "FooRedirection", "foo"); // Not a front article.
  creator.addAlias("foo_ter", "The same redirection", "foo3", {{ zim::writer::FRONT_ARTICLE, true}}); // a clone of the previous redirect, but as a front article.
  creator.addRedirection("foo4", "FooRedirection", "NoExistant", {{zim::writer::FRONT_ARTICLE, true}}); // Invalid redirection, must be removed by creator
  creator.finishZimCreation();

  // Do not use the high level Archive to test that zim file is correctly created but lower structure.
  auto fileCompound = std::make_shared<FileCompound>(tempPath);
  auto reader = std::make_shared<MultiPartFileReader>(fileCompound);
  Fileheader header;
  header.read(*reader);
  ASSERT_TRUE(header.hasMainPage());
#if defined(ENABLE_XAPIAN)
  entry_index_type nb_entry = 13; // counter + 2*illustration + xapiantitleIndex + xapianfulltextIndex + foo + foo2 + foo_bis + foo3 + foo_ter + Title + mainPage + titleListIndexes
  int xapian_mimetype = 0;
  int listing_mimetype = 1;
  int png_mimetype = 2;
  int html_mimetype = 3;
  int plain_mimetype = 4;
  int plainutf8_mimetype = 5;
#else
  entry_index_type nb_entry = 11; // counter + 2*illustration + foo + foo_bis + foo2 + foo3 + foo_ter + Title + mainPage + titleListIndexes
  int listing_mimetype = 0;
  int png_mimetype = 1;
  int html_mimetype = 2;
  int plain_mimetype = 3;
  int plainutf8_mimetype = 4;
#endif

  ASSERT_EQ(header.getArticleCount(), nb_entry);

  // Read dirent
  auto pathPtrReader = reader->sub_reader(offset_t(header.getPathPtrPos()), zsize_t(sizeof(offset_t)*header.getArticleCount()));
  DirectDirentAccessor direntAccessor(std::make_shared<DirentReader>(reader), std::move(pathPtrReader), entry_index_t(header.getArticleCount()));
  std::shared_ptr<const Dirent> dirent;

  entry_index_type direntIdx = 0;
  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_article_dirent(dirent, 'C', "foo", "Foo", html_mimetype, cluster_index_t(0), None);
  auto fooBlobIndex = dirent->getBlobNumber();

  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_article_dirent(dirent, 'C', "foo2", "AFoo", html_mimetype, cluster_index_t(0), None);
  auto foo2BlobIndex = dirent->getBlobNumber();

  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_redirect_dirent(dirent, 'C', "foo3", "FooRedirection", entry_index_t(0));

  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_article_dirent(dirent, 'C', "foo_bis", "The same Foo", html_mimetype, cluster_index_t(0), foo2BlobIndex);

  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_redirect_dirent(dirent, 'C', "foo_ter", "The same redirection", entry_index_t(0));

  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_article_dirent(dirent, 'M', "Counter", None, plain_mimetype, cluster_index_t(0), None);
  auto counterBlobIndex = dirent->getBlobNumber();

  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_article_dirent(dirent, 'M', "Illustration_48x48@1", None, png_mimetype, cluster_index_t(1), None);
  auto illustration48BlobIndex = dirent->getBlobNumber();

  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_article_dirent(dirent, 'M', "Illustration_96x96@1", None, png_mimetype, cluster_index_t(1), None);
  auto illustration96BlobIndex = dirent->getBlobNumber();

  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_article_dirent(dirent, 'M', "Title", "Title", plainutf8_mimetype, cluster_index_t(0), None);
  auto titleBlobIndex = dirent->getBlobNumber();

  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_redirect_dirent(dirent, 'W', "mainPage", "mainPage", entry_index_t(0));

#if defined(ENABLE_XAPIAN)
  dirent = direntAccessor.getDirent(entry_index_t(direntIdx++));
  test_article_dirent(dirent, 'X', "fulltext/xapian", "fulltext/xapian", xapian_mimetype, cluster_index_t(1), None);
#endif

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
  ASSERT_EQ(cluster->getCompression(), Cluster::Compression::Zstd);
  ASSERT_EQ(cluster->count(), blob_index_t(4)); // 4 entries are compressed content

  auto blob = cluster->getBlob(fooBlobIndex);
  ASSERT_EQ(std::string(blob), "FooContent");

  blob = cluster->getBlob(foo2BlobIndex);
  ASSERT_EQ(std::string(blob), "Foo2Content");

  blob = cluster->getBlob(titleBlobIndex);
  ASSERT_EQ(std::string(blob), "This is a title");

  blob = cluster->getBlob(counterBlobIndex);
  ASSERT_EQ(std::string(blob), "text/html=2");


  // Test listing content
  clusterOffset = offset_t(reader->read_uint<offset_type>(offset_t(clusterPtrPos + 8)));
  cluster = Cluster::read(*reader, clusterOffset);
  ASSERT_EQ(cluster->getCompression(), Cluster::Compression::None);
  ASSERT_EQ(cluster->count(), blob_index_t(nb_entry-8)); // 7 entries are either compressed or redirections + 1 entry is a clone of content

  ASSERT_EQ(header.getTitleIdxPos(), 0);

  blob = cluster->getBlob(v1BlobIndex);
  ASSERT_EQ(blob.size(), 3*sizeof(title_index_t));
  std::vector<char> blob1Data(blob.data(), blob.end());
  std::vector<char> expectedBlob1Data = {
    1, 0, 0, 0,
    0, 0, 0, 0,
    4, 0, 0, 0 // "The same redirection" is the 5th entry "by title order"
  };
  ASSERT_EQ(blob1Data, expectedBlob1Data);

  blob = cluster->getBlob(illustration48BlobIndex);
  ASSERT_EQ(std::string(blob), "PNGBinaryContent48");

  blob = cluster->getBlob(illustration96BlobIndex);
  ASSERT_EQ(std::string(blob), "PNGBinaryContent96");
}


TEST(ZimCreator, interruptedZimCreation)
{
  unittests::TempFile tmpFile("zimfile");
  {
    writer::Creator creator;
    creator.configClusterSize(16*1024);
    creator.startZimCreation(tmpFile.path());
    zim::Formatter fmt;
    for ( size_t i = 0; i < 12345; ++i ) {
      fmt << i;
    }
    const std::string content(fmt);
    for ( char c = 'a'; c <= 'z'; ++c ) {
      const std::string path(1, c);
      creator.addItem(std::make_shared<TestItem>(path, path, content));
    }
    // creator.finishZimCreation() is not called
  }

  EXPECT_THROW(
      {
        const zim::Archive archive(tmpFile.path());
      },
      zim::ZimFileFormatError
  );
}


} // unnamed namespace
