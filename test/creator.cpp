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
#include <zim/item.h>
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
using zim::unittests::CapturedStdout;

#define ASSERT_REDIRECT_ENTRY(archive, path, title, targetPath) \
{\
  ASSERT_NO_THROW(archive.getEntryByPath(path)); \
  const auto entry = archive.getEntryByPath(path); \
  ASSERT_TRUE(entry.isRedirect()); \
  ASSERT_EQ(entry.getTitle(), title); \
  ASSERT_EQ(entry.getRedirectEntry().getPath(), targetPath); \
}

#define ASSERT_ITEM_ENTRY(archive, path, title, mimetype, content) \
{\
  ASSERT_NO_THROW(archive.getEntryByPath(path));   \
  const auto entry = archive.getEntryByPath(path); \
  ASSERT_FALSE(entry.isRedirect());                \
  const auto item = entry.getItem();               \
  ASSERT_EQ(item.getTitle(), title);               \
  ASSERT_EQ(item.getMimetype(), mimetype);         \
  ASSERT_EQ(std::string(item.getData()), content); \
}

#define EXPECT_MISSING_ENTRY(archive, path)  \
  EXPECT_THROW(archive.getEntryByPath(path), zim::EntryNotFound)

struct NoneType {};
const NoneType None;

zim::Uuid makeSafeUuid() {
  zim::Uuid uuid;
  // Force special char in the uuid to be sure they are not handled particularly
  uuid.data[5] = '\n';
  uuid.data[10] = '\0';
  return uuid;
}

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

  writer::Creator creator;
  creator.setUuid(makeSafeUuid());
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

std::shared_ptr<zim::writer::Item> makeTestItem(const std::string& path,
                                                const std::string& title,
                                                const std::string& content)
{
  return std::make_shared<TestItem>(path, title, content);
}

TEST(ZimCreator, createZim)
{
  unittests::TempFile temp("zimfile");
  auto tempPath = temp.path();

  writer::Creator creator;
  creator.setUuid(makeSafeUuid());
  creator.configIndexing(true, "eng");
  creator.startZimCreation(tempPath);
  auto item = makeTestItem("foo", "Foo", "FooContent");
  EXPECT_NO_THROW(creator.addItem(item));
  EXPECT_THROW(creator.addItem(item), std::runtime_error);
  // Be sure that title order is not the same that path order
  item = makeTestItem("foo2", "AFoo", "Foo2Content");
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

  ASSERT_EQ(header.getTitleIdxPos(), 0xffffffffffffffffUL);

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
      creator.addItem(makeTestItem(path, path, content));
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

// Output produced by ZIM creation when no invalid redirects are left behind
const std::string OUTPUT_FROM_TIDY_ZIM_CREATION =
    "Resolve redirect\n"
    "Detect loops and/or blind chains of redirects\n"
    "set index\n";

TEST(ZimCreator, redirectAddedAfterItsTargetIsCreated)
{
  unittests::TempFile temp("zimfile");
  const auto tempPath = temp.path();

  CapturedStdout stdOut;

  writer::Creator creator;
  creator.setUuid(makeSafeUuid());
  creator.startZimCreation(tempPath);

  creator.addItem(makeTestItem("item", "An item", "<html/>"));
  creator.addRedirection("redirect", "A redirect", "item");

  creator.finishZimCreation();
  EXPECT_EQ(stdOut.str(), OUTPUT_FROM_TIDY_ZIM_CREATION);

  const zim::Archive a(tempPath);
  ASSERT_ITEM_ENTRY(a, "item",  "An item",  "text/html", "<html/>");
  ASSERT_REDIRECT_ENTRY(a, "redirect", "A redirect", "item");
}

TEST(ZimCreator, redirectAddedBeforeItsTargetIsCreated)
{
  unittests::TempFile temp("zimfile");
  const auto tempPath = temp.path();

  CapturedStdout stdOut;

  writer::Creator c;
  c.setUuid(makeSafeUuid());
  c.startZimCreation(tempPath);

  // Add redirects with targets not yet created
  c.addRedirection("r1", "Redirect to an item",          "item");
  c.addRedirection("r2", "Redirect to an item alias",    "item_alias");
  c.addRedirection("r3", "Redirect to another redirect", "redirect");
  c.addRedirection("r4", "Redirect to a redirect alias", "redir_alias");

  // Add entries serving as targets for the redirects from the previous block
  c.addItem(makeTestItem("item", "An item", ""));
  c.addAlias("item_alias", "An alias of an item", "item");
  c.addRedirection("redirect", "Safe redirect to an item", "item");
  c.addAlias("redir_alias", "An alias of a redirect", "redirect");

  c.finishZimCreation();
  EXPECT_EQ(stdOut.str(), OUTPUT_FROM_TIDY_ZIM_CREATION);

  const zim::Archive a(tempPath);
  ASSERT_ITEM_ENTRY(a, "item",  "An item",  "text/html", "");
  ASSERT_ITEM_ENTRY(a, "item_alias",  "An alias of an item",  "text/html", "");
  ASSERT_REDIRECT_ENTRY(a, "redirect", "Safe redirect to an item", "item");
  ASSERT_REDIRECT_ENTRY(a, "redir_alias", "An alias of a redirect", "item");

  ASSERT_REDIRECT_ENTRY(a, "r1", "Redirect to an item",          "item");
  ASSERT_REDIRECT_ENTRY(a, "r2", "Redirect to an item alias",    "item_alias");
  ASSERT_REDIRECT_ENTRY(a, "r3", "Redirect to another redirect", "redirect");
  ASSERT_REDIRECT_ENTRY(a, "r4", "Redirect to a redirect alias", "redir_alias");
}


TEST(ZimCreator, aliases)
{
  unittests::TempFile temp("zimfile");
  const auto tempPath = temp.path();

  CapturedStdout stdOut;

  writer::Creator creator;
  creator.setUuid(makeSafeUuid());
  creator.startZimCreation(tempPath);

  creator.addItem(makeTestItem("item", "An item", "<html/>"));
  creator.addRedirection("redirect", "A redirect", "item");

  creator.addAlias("item_alias",     "An item alias",    "item");
  creator.addAlias("redirect_alias", "A redirect alias", "redirect");

  EXPECT_THROW(
    creator.addAlias("a_dangling_alias", "A dangling alias", "no_such_entry"),
    InvalidEntry
  );

  EXPECT_THROW(
    creator.addAlias("self_alias", "A self-referencing alias", "self_alias"),
    InvalidEntry
  );

  creator.finishZimCreation();
  EXPECT_EQ(stdOut.str(), OUTPUT_FROM_TIDY_ZIM_CREATION);

  const zim::Archive a(tempPath);
  ASSERT_ITEM_ENTRY(a, "item",  "An item",  "text/html", "<html/>");
  ASSERT_ITEM_ENTRY(a, "item_alias", "An item alias", "text/html", "<html/>");
  ASSERT_REDIRECT_ENTRY(a, "redirect", "A redirect", "item");
  ASSERT_REDIRECT_ENTRY(a, "redirect_alias", "A redirect alias", "item");
}

TEST(ZimCreator, attemptingToOverwriteAnItemWithAnotherItem)
{
  unittests::TempFile temp("zimfile");
  const auto tempPath = temp.path();

  CapturedStdout stdOut;

  writer::Creator creator;
  creator.setUuid(makeSafeUuid());
  creator.startZimCreation(tempPath);

  creator.addItem(makeTestItem("path", "An item", "I tempt you"));
  EXPECT_THROW(
      creator.addItem(makeTestItem("path", "Another item", "It embodies me")),
      InvalidEntry
  );

  creator.finishZimCreation();
  EXPECT_EQ(stdOut.str(), OUTPUT_FROM_TIDY_ZIM_CREATION);

  const zim::Archive archive(tempPath);
  ASSERT_ITEM_ENTRY(archive, "path", "An item", "text/html", "I tempt you");
}

TEST(ZimCreator, attemptingToOverwriteARedirectWithAnotherRedirect)
{
  unittests::TempFile temp("zimfile");
  const auto tempPath = temp.path();

  CapturedStdout stdOut;

  writer::Creator creator;
  creator.setUuid(makeSafeUuid());
  creator.startZimCreation(tempPath);

  creator.addItem(makeTestItem("itempath", "An item", "I tempt you"));
  creator.addRedirection("redirectpath", "A redirect", "itempath");
  EXPECT_THROW(
      creator.addRedirection("redirectpath", "Another redirect", "itempath"),
      InvalidEntry
  );
  EXPECT_THROW(
      creator.addRedirection("redirectpath", "A redirect", "anotherpath"),
      InvalidEntry
  );

  creator.finishZimCreation();
  EXPECT_EQ(stdOut.str(), OUTPUT_FROM_TIDY_ZIM_CREATION);

  const zim::Archive archive(tempPath);
  ASSERT_ITEM_ENTRY(archive, "itempath", "An item", "text/html", "I tempt you");
  ASSERT_REDIRECT_ENTRY(archive, "redirectpath", "A redirect", "itempath");
}

TEST(ZimCreator, attemptingToOverwriteARedirectWithAnItem)
{
  unittests::TempFile temp("zimfile");
  const auto tempPath = temp.path();

  CapturedStdout stdOut;

  writer::Creator creator;
  creator.setUuid(makeSafeUuid());
  creator.startZimCreation(tempPath);

  creator.addItem(makeTestItem("path1", "Item#1", "I tempt you"));

  creator.addRedirection("path2", "A redirect", "path1");
  creator.addRedirection("path3", "Second level redirect", "path2");

  ASSERT_THROW(
    creator.addItem(makeTestItem("path2", "Item#2", "I tempt you too")),
    InvalidEntry
  );

  creator.finishZimCreation();
  EXPECT_EQ(stdOut.str(), OUTPUT_FROM_TIDY_ZIM_CREATION);

  const zim::Archive archive(tempPath);
  ASSERT_ITEM_ENTRY(archive, "path1", "Item#1", "text/html", "I tempt you");
  ASSERT_REDIRECT_ENTRY(archive, "path2", "A redirect", "path1");
  ASSERT_REDIRECT_ENTRY(archive, "path3", "Second level redirect", "path2");
}

TEST(ZimCreator, attemptingToOverwriteAnItemWithARedirect)
{
  unittests::TempFile temp("zimfile");
  const auto tempPath = temp.path();

  CapturedStdout stdOut;

  writer::Creator creator;
  creator.setUuid(makeSafeUuid());
  creator.startZimCreation(tempPath);

  creator.addItem(makeTestItem("path", "An item", "I tempt you"));
  EXPECT_THROW(
      creator.addRedirection("path", "A redirect", "anotherpath"),
      InvalidEntry
  );

  creator.finishZimCreation();
  EXPECT_EQ(stdOut.str(), OUTPUT_FROM_TIDY_ZIM_CREATION);

  const zim::Archive archive(tempPath);
  ASSERT_ITEM_ENTRY(archive, "path", "An item", "text/html", "I tempt you");
}

TEST(ZimCreator, attemptingToOverwriteAnExistingEntryWithAnAlias)
{
  unittests::TempFile temp("zimfile");
  const auto tempPath = temp.path();

  CapturedStdout stdOut;

  writer::Creator creator;
  creator.setUuid(makeSafeUuid());
  creator.startZimCreation(tempPath);

  creator.addItem(makeTestItem("item1", "Item#1", "It empowers everyone"));
  creator.addItem(makeTestItem("item2", "Item#2", "It embarrasses us"));
  creator.addRedirection("redir1", "Redirect#1", "item1");
  creator.addRedirection("redir2", "Redirect#2", "item2");

  ASSERT_THROW( // try to overwrite an item with an alias of another item
    creator.addAlias("item2", "Item#2 is just an alias for Item#1", "item1"),
    InvalidEntry
  );

  ASSERT_THROW( // try to overwrite an item with an alias of itself
    creator.addAlias("item1", "If it works, should be a no-op", "item1"),
    InvalidEntry
  );

  ASSERT_THROW( // try to overwrite an item with an alias of a redirect
    creator.addAlias("item1", "Let item2 be a redirect instead", "redir1"),
    InvalidEntry
  );

  ASSERT_THROW( // try to overwrite a redirect with an alias of another redirect
    creator.addAlias("redir2", "Let redirects 2 & 1 be one", "redir1"),
    InvalidEntry
  );

  ASSERT_THROW( // try to overwrite a redirect with an alias of itself
    creator.addAlias("redir1", "If it works, should be a no-op", "redir1"),
    InvalidEntry
  );

  ASSERT_THROW( // try to overwrite a redirect with an alias of an item
    creator.addAlias("redir2", "Make redir2 an alias of item1", "item1"),
    InvalidEntry
  );

  creator.finishZimCreation();
  EXPECT_EQ(stdOut.str(), OUTPUT_FROM_TIDY_ZIM_CREATION);

  const zim::Archive a(tempPath);
  ASSERT_ITEM_ENTRY(a, "item1", "Item#1", "text/html", "It empowers everyone");
  ASSERT_ITEM_ENTRY(a, "item2", "Item#2", "text/html", "It embarrasses us");
  ASSERT_REDIRECT_ENTRY(a, "redir1", "Redirect#1", "item1");
  ASSERT_REDIRECT_ENTRY(a, "redir2", "Redirect#2", "item2");
}

TEST(ZimCreator, handlingOfAnAscendingBlindChainOfRedirections)
{
  unittests::TempFile temp("zimfile");
  const auto tempPath = temp.path();

  CapturedStdout stdOut;

  writer::Creator creator;
  creator.setUuid(makeSafeUuid());
  creator.startZimCreation(tempPath);

  creator.addItem(makeTestItem("1st", "1st entry in ZIM", ""));

  // Create a blind ascending chain of redirects, i.e. a chain
  // of redirects where the last entry of the chain is an invalid redirect
  // and the rest are of the form (path1 -> path2) where path1 < path2
  creator.addRedirection("redirectA",
                         "First redirect in an ascending blind chain",
                         "redirectB");

  creator.addRedirection("redirectB",
                         "Middle redirect in an ascending blind chain",
                         "redirectC");

  creator.addRedirection("redirectC",
                         "Last redirect in an ascending blind chain",
                         "missingTarget");

  EXPECT_NO_THROW(creator.finishZimCreation());

  EXPECT_EQ(stdOut.str(),
    "Resolve redirect\n"
    "Removing invalid redirection C/redirectC redirecting to (missing) C/missingTarget\n"
    "Detect loops and/or blind chains of redirects\n"
    "Redirection C/redirectA -> C/redirectB belongs to a blind chain or loop. Removing...\n"
    "Redirection C/redirectB -> C/redirectC belongs to a blind chain or loop. Removing...\n"
    "set index\n"
  );

  const zim::Archive archive(tempPath);

  EXPECT_MISSING_ENTRY(archive, "missingTarget");
  EXPECT_MISSING_ENTRY(archive, "redirectA");
  EXPECT_MISSING_ENTRY(archive, "redirectB");
  EXPECT_MISSING_ENTRY(archive, "redirectC");
}

TEST(ZimCreator, handlingOfADescendingBlindChainOfRedirections)
{
  unittests::TempFile temp("zimfile");
  const auto tempPath = temp.path();

  CapturedStdout stdOut;

  writer::Creator creator;
  creator.setUuid(makeSafeUuid());
  creator.startZimCreation(tempPath);

  creator.addItem(makeTestItem("1st", "1st entry in ZIM", ""));

  // Create a blind descending chain of redirects, i.e. a chain
  // of redirects where the last entry of the chain is an invalid redirect
  // and the rest are of the form (path1 -> path2) where path1 > path2
  creator.addRedirection("redirectC",
                         "First redirect in a descending blind chain",
                         "redirectB");

  creator.addRedirection("redirectB",
                         "Middle redirect in a descending blind chain",
                         "redirectA");

  creator.addRedirection("redirectA",
                         "Last redirect in a descending blind chain",
                         "missingTarget");

  EXPECT_NO_THROW(creator.finishZimCreation());

  EXPECT_EQ(stdOut.str(),
    "Resolve redirect\n"
    "Removing invalid redirection C/redirectA redirecting to (missing) C/missingTarget\n"
    "Detect loops and/or blind chains of redirects\n"
    "Redirection C/redirectB -> C/redirectA belongs to a blind chain or loop. Removing...\n"
    "Redirection C/redirectC -> C/redirectB belongs to a blind chain or loop. Removing...\n"
    "set index\n"
  );

  const zim::Archive archive(tempPath);

  EXPECT_MISSING_ENTRY(archive, "missingTarget");
  EXPECT_MISSING_ENTRY(archive, "redirectA");
  EXPECT_MISSING_ENTRY(archive, "redirectB");
  EXPECT_MISSING_ENTRY(archive, "redirectC");
}

TEST(ZimCreator, handlingOfRedirectionLoops)
{
  unittests::TempFile temp("zimfile");
  const auto tempPath = temp.path();

  CapturedStdout stdOut;

  writer::Creator creator;
  creator.setUuid(makeSafeUuid());
  creator.startZimCreation(tempPath);

  creator.addRedirection("redirectA", "A -> B", "redirectB");
  creator.addRedirection("redirectB", "B -> C", "redirectC");
  creator.addRedirection("redirectC", "C -> D", "redirectD");
  creator.addRedirection("redirectD", "D -> B", "redirectB");

  creator.addRedirection("redirectS", "S -> S", "redirectS");

  creator.addRedirection("redirectZ", "Z -> Y", "redirectY");
  creator.addRedirection("redirectY", "Y -> X", "redirectX");
  creator.addRedirection("redirectX", "X -> W", "redirectW");
  creator.addRedirection("redirectW", "W -> Y", "redirectY");
  creator.finishZimCreation();

  EXPECT_EQ(stdOut.str(),
    "Resolve redirect\n"
    "Detect loops and/or blind chains of redirects\n"
    "Redirection C/redirectA -> C/redirectB belongs to a blind chain or loop. Removing...\n"
    "Redirection C/redirectB -> C/redirectC belongs to a blind chain or loop. Removing...\n"
    "Redirection C/redirectC -> C/redirectD belongs to a blind chain or loop. Removing...\n"
    "Redirection C/redirectD -> C/redirectB belongs to a blind chain or loop. Removing...\n"
    "Redirection C/redirectS -> C/redirectS belongs to a blind chain or loop. Removing...\n"
    "Redirection C/redirectW -> C/redirectY belongs to a blind chain or loop. Removing...\n"
    "Redirection C/redirectY -> C/redirectX belongs to a blind chain or loop. Removing...\n"
    "Redirection C/redirectX -> C/redirectW belongs to a blind chain or loop. Removing...\n"
    "Redirection C/redirectZ -> C/redirectY belongs to a blind chain or loop. Removing...\n"
    "set index\n"
  );

  const zim::Archive archive(tempPath);

  EXPECT_MISSING_ENTRY(archive, "redirectA");
  EXPECT_MISSING_ENTRY(archive, "redirectB");
  EXPECT_MISSING_ENTRY(archive, "redirectC");
  EXPECT_MISSING_ENTRY(archive, "redirectD");

  EXPECT_MISSING_ENTRY(archive, "redirectS");

  EXPECT_MISSING_ENTRY(archive, "redirectW");
  EXPECT_MISSING_ENTRY(archive, "redirectX");
  EXPECT_MISSING_ENTRY(archive, "redirectY");
  EXPECT_MISSING_ENTRY(archive, "redirectZ");
}

TEST(ZimCreator, pruningOfARedirectionForest)
{
  unittests::TempFile temp("zimfile");
  const auto tempPath = temp.path();

  CapturedStdout stdOut;

  writer::Creator creator;
  creator.setUuid(makeSafeUuid());
  creator.startZimCreation(tempPath);

  // Create redirections according to the following diagram (for convenience
  // uppercase letters and digits participate only in invalid redirection
  // components, while the valid redirection tree is built exclusively of
  // lowercase letters):
  //
  //      T    P <- O
  //      |    |    ^
  //      v    v    |       // A tree leading into a redirection loop
  // M -> N -> L -> 0
  //
  // A -> B
  //        \                   //
  // J -> K ---> Q -> (MISSING) // A tree ending with a dangling redirect
  //        /                   //
  // Z -> Y
  //
  // a -> b
  //        \               // This one is the only healthy redirection tree
  // j -> k ---> q -> root  // in this diagram. Only redirections from this
  //        /               // component should survive.
  // z -> y
  //

  creator.addItem(makeTestItem("root", "The only item", "ROOT"));

  // A tree leading into a redirection loop
  creator.addRedirection("T", "T -> N", "N");
  creator.addRedirection("M", "M -> N", "N");
  creator.addRedirection("N", "N -> L", "L");
  creator.addRedirection("L", "L -> 0", "0");
  creator.addRedirection("0", "0 -> O", "O");
  creator.addRedirection("O", "O -> P", "P");
  creator.addRedirection("P", "P -> L", "L");

  // A redirection tree ending with a dangling redirect
  creator.addRedirection("A", "A -> B", "B");
  creator.addRedirection("B", "B -> Q", "Q");
  creator.addRedirection("J", "J -> K", "K");
  creator.addRedirection("K", "K -> Q", "Q");
  creator.addRedirection("Z", "Z -> Y", "Y");
  creator.addRedirection("Y", "Y -> Q", "Q");
  creator.addRedirection("Q", "Q -> (MISSING)", "(MISSING)");

  // A redirection tree rooted at an item entry
  creator.addRedirection("a", "a -> b", "b");
  creator.addRedirection("b", "b -> q", "q");
  creator.addRedirection("j", "j -> k", "k");
  creator.addRedirection("k", "k -> q", "q");
  creator.addRedirection("z", "z -> y", "y");
  creator.addRedirection("y", "y -> q", "q");
  creator.addRedirection("q", "q -> root", "root");

  creator.finishZimCreation();

  EXPECT_EQ(stdOut.str(),
    "Resolve redirect\n"
    "Removing invalid redirection C/Q redirecting to (missing) C/(MISSING)\n"
    "Detect loops and/or blind chains of redirects\n"
    "Redirection C/0 -> C/O belongs to a blind chain or loop. Removing...\n"
    "Redirection C/O -> C/P belongs to a blind chain or loop. Removing...\n"
    "Redirection C/P -> C/L belongs to a blind chain or loop. Removing...\n"
    "Redirection C/L -> C/0 belongs to a blind chain or loop. Removing...\n"
    "Redirection C/A -> C/B belongs to a blind chain or loop. Removing...\n"
    "Redirection C/B -> C/Q belongs to a blind chain or loop. Removing...\n"
    "Redirection C/J -> C/K belongs to a blind chain or loop. Removing...\n"
    "Redirection C/K -> C/Q belongs to a blind chain or loop. Removing...\n"
    "Redirection C/M -> C/N belongs to a blind chain or loop. Removing...\n"
    "Redirection C/N -> C/L belongs to a blind chain or loop. Removing...\n"
    "Redirection C/T -> C/N belongs to a blind chain or loop. Removing...\n"
    "Redirection C/Y -> C/Q belongs to a blind chain or loop. Removing...\n"
    "Redirection C/Z -> C/Y belongs to a blind chain or loop. Removing...\n"
    "set index\n"
  );

  const zim::Archive archive(tempPath);

  EXPECT_MISSING_ENTRY(archive, "(MISSING)");
  EXPECT_MISSING_ENTRY(archive, "0");
  for ( char c = 'A'; c <= 'Z'; ++c ) {
    // The constructor of std::string taking a character and a count
    // is prone to the mistake of supplying the arguments in the wrong order
    // (which doesn't lead to a compilation error).
    // Hence using this more explicit way of converting a char to a string.
    std::string path; path.push_back(c);
    EXPECT_MISSING_ENTRY(archive, path);
  }

  ASSERT_ITEM_ENTRY(archive, "root", "The only item", "text/html", "ROOT");
  ASSERT_REDIRECT_ENTRY(archive, "a", "a -> b", "b");
  ASSERT_REDIRECT_ENTRY(archive, "b", "b -> q", "q");
  ASSERT_REDIRECT_ENTRY(archive, "j", "j -> k", "k");
  ASSERT_REDIRECT_ENTRY(archive, "k", "k -> q", "q");
  ASSERT_REDIRECT_ENTRY(archive, "z", "z -> y", "y");
  ASSERT_REDIRECT_ENTRY(archive, "y", "y -> q", "q");
  ASSERT_REDIRECT_ENTRY(archive, "q", "q -> root", "root");
}

struct SimulatedError : std::exception
{
};

std::shared_ptr<zim::writer::Item> makeUnaddableItem(const std::string& path)
{
  struct UnaddableItem : TestItem
  {
    UnaddableItem(const std::string& path) : TestItem(path, "", "") {}
    virtual std::unique_ptr<writer::ContentProvider> getContentProvider() const
    {
      throw SimulatedError();
    }
  };

  return std::make_shared<UnaddableItem>(path);
}

TEST(ZimCreator, addingATargetForAnAlreadyAddedRedirectFails)
{
  unittests::TempFile temp("zimfile");
  auto tempPath = temp.path();

  CapturedStdout stdOut;
  writer::Creator c;
  c.setUuid(makeSafeUuid());
  c.startZimCreation(tempPath);

  c.addRedirection("R1", "Redirect to a discardable item", "Discardable");
  c.addRedirection("R2", "Redirect to an important item", "Important");

  // Adding this item fails and is not retried
  EXPECT_THROW(c.addItem(makeUnaddableItem("Discardable")), SimulatedError);

  // Adding this item fails but is retried with a success
  EXPECT_THROW(c.addItem(makeUnaddableItem("Important")), SimulatedError);
  EXPECT_NO_THROW(c.addItem(makeTestItem("Important", "", "")));

  EXPECT_NO_THROW(c.finishZimCreation());

  ASSERT_EQ(stdOut.str(),
    "Resolve redirect\n"
    "Removing invalid redirection C/R1 redirecting to (missing) C/Discardable\n"
    "Detect loops and/or blind chains of redirects\n"
    "set index\n"
  );

  const zim::Archive a(tempPath);

  EXPECT_MISSING_ENTRY(a, "R1");
  EXPECT_MISSING_ENTRY(a, "Discardable");
  ASSERT_REDIRECT_ENTRY(a, "R2", "Redirect to an important item", "Important");
}

} // unnamed namespace
