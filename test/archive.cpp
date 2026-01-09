/*
 * Copyright (C) 2020 Veloman Yunkan
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

#define ZIM_PRIVATE
#include <zim/zim.h>
#include <zim/archive.h>
#include <zim/illustration.h>
#include <zim/item.h>
#if defined(ENABLE_XAPIAN)
#include <zim/search.h>
#endif
#include <zim/suggestion.h>
#include <zim/error.h>

#include <zim/writer/creator.h>

#include "tools.h"
#include "../src/fs.h"

#include "gtest/gtest.h"

namespace
{

using zim::unittests::makeTempFile;
using zim::unittests::getDataFilePath;
using zim::unittests::TempFile;
using zim::unittests::TestItem;
using zim::unittests::IsFrontArticle;

class ZimArchive: public testing::Test {
  protected:
  void SetUp() override {
    zim::setClusterCacheMaxSize(0);
    zim::setClusterCacheMaxSize(CLUSTER_CACHE_SIZE);
    ASSERT_EQ(zim::getClusterCacheCurrentSize(), 0);
  }
  void TearDown() override {
    ASSERT_EQ(zim::getClusterCacheCurrentSize(), 0);
  }
};

using TestContextImpl = std::vector<std::pair<std::string, std::string> >;
struct TestContext : TestContextImpl {
  TestContext(const std::initializer_list<value_type>& il)
    : TestContextImpl(il)
  {}
};

std::ostream& operator<<(std::ostream& out, const TestContext& ctx)
{
  out << "Test context:\n";
  for ( const auto& kv : ctx )
    out << "\t" << kv.first << ": " << kv.second << "\n";
  out << std::endl;
  return out;
}

std::string
emptyZimArchiveContent()
{
  std::string content;
  content += "ZIM\x04"; // Magic
  content += "\x05" + std::string(3, '\0'); // Version
  content += std::string(16, '\0'); // uuid
  content += std::string(4, '\0'); // article count
  content += std::string(4, '\0'); // cluster count
  content += "\x51" + std::string(7, '\0'); // url ptr pos
  content += "\x51" + std::string(7, '\0'); // title ptr pos
  content += "\x51" + std::string(7, '\0'); // cluster ptr pos
  content += "\x50" + std::string(7, '\0'); // mimelist ptr pos
  content += std::string(4, '\0'); // main page index
  content += std::string(4, '\0'); // layout page index
  content += "\x51" + std::string(7, '\0'); // checksum pos
  content += std::string(1, '\0');; // (empty) mimelist
  content += "\x9f\x3e\xcd\x95\x46\xf6\xc5\x3b\x35\xb4\xc6\xd4\xc0\x8e\xd0\x66"; // md5sum
  return content;
}

TEST_F(ZimArchive, openingAnInvalidZimArchiveFails)
{
  const char* const prefixes[] = { "ZIM\x04", "" };
  const unsigned char bytes[] = {0x00, 0x01, 0x11, 0x30, 0xFF};
  for ( const std::string prefix : prefixes ) {
    for ( const unsigned char byte : bytes ) {
      for ( int count = 0; count < 100; count += 10 ) {
        const TestContext ctx{
                {"prefix",  prefix.size() ? "yes" : "no" },
                {"byte", zim::unittests::to_string(byte) },
                {"count", zim::unittests::to_string(count) }
        };
        const std::string zimfileContent = prefix + std::string(count, byte);
        const auto tmpfile = makeTempFile("invalid_zim_file", zimfileContent);

        EXPECT_THROW( zim::Archive(tmpfile->path()), std::runtime_error ) << ctx;
      }
    }
  }
}

TEST_F(ZimArchive, openingAnEmptyZimArchiveSucceeds)
{
  const auto tmpfile = makeTempFile("empty_zim_file", emptyZimArchiveContent());

  zim::Archive archive(tmpfile->path());
  ASSERT_TRUE(archive.check());

  ASSERT_EQ(archive.getMediaCount(), 0U);
  ASSERT_EQ(archive.getArticleCount(), 0U);
}

bool isNastyOffset(int offset) {
  if ( 6 <= offset && offset < 24 ) // Minor version or uuid
    return false;

  if ( 64 <= offset && offset < 72 ) // page or layout index
    return false;

  return true;
}

TEST_F(ZimArchive, nastyEmptyZimArchive)
{
  const std::string correctContent = emptyZimArchiveContent();
  for ( int offset = 0; offset < 80; ++offset ) {
    if ( isNastyOffset(offset) ) {
      const TestContext ctx{ {"offset", zim::unittests::to_string(offset) } };
      std::string nastyContent(correctContent);
      nastyContent[offset] = '\xff';
      const auto tmpfile = makeTempFile("wrong_checksum_empty_zim_file", nastyContent);
      EXPECT_THROW( zim::Archive(tmpfile->path()), std::runtime_error ) << ctx;
    }
  }
}

TEST_F(ZimArchive, wrongChecksumInEmptyZimArchive)
{
  std::string zimfileContent = emptyZimArchiveContent();
  zimfileContent[85] = '\xff';
  const auto tmpfile = makeTempFile("wrong_checksum_empty_zim_file", zimfileContent);

  zim::Archive archive(tmpfile->path());
  ASSERT_FALSE(archive.check());
}


TEST_F(ZimArchive, openCreatedArchive)
{
  TempFile temp("zimfile");
  auto tempPath = temp.path();
  zim::Uuid uuid;
  // Force special char in the uuid to be sure they are not handled particularly.
  uuid.data[5] = '\n';
  uuid.data[10] = '\0';

  zim::writer::Creator creator;
  creator.setUuid(uuid);
  creator.configIndexing(true, "eng");
  creator.startZimCreation(tempPath);
  auto item = std::make_shared<TestItem>("foo", "text/html", "Foo", "FooContent", IsFrontArticle::YES);
  creator.addItem(item);
  // Be sure that title order is not the same that url order
  item = std::make_shared<TestItem>("foo2", "text/html", "AFoo", "Foo2Content", IsFrontArticle::NO);
  creator.addItem(item);
  creator.addMetadata("Title", "This is a title");
  creator.addIllustration(zim::IllustrationInfo{48, 48, 1, {}}, "PNGBinaryContent48");
  creator.addIllustration(zim::IllustrationInfo{96, 96, 1, {}}, "PNGBinaryContent96");
  creator.setMainPath("foo");
  creator.addRedirection("foo3", "FooRedirection", "foo"); // No a front article.
  creator.addRedirection("foo4", "FooRedirection", "NoExistant"); // Invalid redirection, must be removed by creator
  creator.finishZimCreation();

  zim::Archive archive(tempPath);
#if !defined(ENABLE_XAPIAN)
// listingIndex + M/Counter + M/Title + mainpage + 2*Illustration + 2*Item + redirection
#define ALL_ENTRY_COUNT 9U
#else
// same as above + 2 xapian indexes.
#define ALL_ENTRY_COUNT 11U
#endif
  ASSERT_EQ(archive.getAllEntryCount(), ALL_ENTRY_COUNT);
#undef ALL_ENTRY_COUNT
  ASSERT_EQ(archive.getEntryCount(), 3U);
  ASSERT_EQ(archive.getArticleCount(), 1U);
  ASSERT_EQ(archive.getUuid(), uuid);
  ASSERT_EQ(archive.getMetadataKeys(), std::vector<std::string>({"Counter", "Illustration_48x48@1", "Illustration_96x96@1", "Title"}));
  ASSERT_EQ(archive.getIllustrationInfos(), zim::Archive::IllustrationInfos({
              {48, 48, 1.0, {}},
              {96, 96, 1.0, {}},
  }));
  ASSERT_EQ(archive.getIllustrationInfos(48, 48, 1), zim::Archive::IllustrationInfos({
              {48, 48, 1.0, {}},
  }));
  ASSERT_TRUE(archive.hasMainEntry());

  ASSERT_EQ(archive.getMetadata("Title"), "This is a title");
  auto titleMeta = archive.getMetadataItem("Title");
  ASSERT_EQ(std::string(titleMeta.getData()), "This is a title");
  ASSERT_EQ(titleMeta.getMimetype(), "text/plain;charset=utf-8");

  auto titleMeta_with_ns = archive.getEntryByPathWithNamespace('M', "Title");
  ASSERT_EQ(titleMeta.getIndex(), titleMeta_with_ns.getIndex());

  ASSERT_EQ(archive.getMetadata("Counter"), "text/html=2");

  auto illu48 = archive.getIllustrationItem(zim::IllustrationInfo{48, 48, 1.0, {}});
  ASSERT_EQ(illu48.getPath(), "Illustration_48x48@1");
  ASSERT_EQ(std::string(illu48.getData()), "PNGBinaryContent48");
  auto illu48Meta = archive.getMetadataItem(illu48.getPath());
  ASSERT_EQ(std::string(illu48Meta.getData()), "PNGBinaryContent48");
  ASSERT_EQ(illu48Meta.getMimetype(), "image/png");
  auto illu96 = archive.getIllustrationItem(zim::IllustrationInfo{96, 96, 1.0, {}});
  ASSERT_EQ(illu96.getPath(), "Illustration_96x96@1");
  ASSERT_EQ(std::string(illu96.getData()), "PNGBinaryContent96");

  auto foo = archive.getEntryByPath("foo");
  ASSERT_EQ(foo.getPath(), "foo");
  ASSERT_EQ(foo.getTitle(), "Foo");
  ASSERT_EQ(std::string(foo.getItem().getData()), "FooContent");
  ASSERT_THROW(foo.getRedirectEntry(), zim::InvalidType);
  ASSERT_THROW(foo.getRedirectEntryIndex(), zim::InvalidType);

  auto foo_with_ns = archive.getEntryByPathWithNamespace('C', "foo");
  ASSERT_EQ(foo.getIndex(), foo_with_ns.getIndex());

  auto foo2 = archive.getEntryByPath("foo2");
  ASSERT_EQ(foo2.getPath(), "foo2");
  ASSERT_EQ(foo2.getTitle(), "AFoo");
  ASSERT_EQ(std::string(foo2.getItem().getData()), "Foo2Content");

  auto foo3 = archive.getEntryByPath("foo3");
  ASSERT_EQ(foo3.getPath(), "foo3");
  ASSERT_EQ(foo3.getTitle(), "FooRedirection");
  ASSERT_TRUE(foo3.isRedirect());
  ASSERT_EQ(foo3.getRedirectEntry().getIndex(), foo.getIndex());
  ASSERT_EQ(foo3.getRedirectEntryIndex(), foo.getIndex());

  auto main = archive.getMainEntry();
  ASSERT_TRUE(main.isRedirect());
  ASSERT_EQ(main.getRedirectEntry().getIndex(), foo.getIndex());
  ASSERT_EQ(main.getRedirectEntryIndex(), foo.getIndex());
  ASSERT_EQ(archive.getMainEntryIndex(), main.getIndex());

  // NO existant entries
  ASSERT_THROW(archive.getEntryByPath("non/existant/path"), zim::EntryNotFound);
  ASSERT_THROW(archive.getEntryByPath("C/non/existant/path"), zim::EntryNotFound);
  ASSERT_THROW(archive.getEntryByPathWithNamespace('C', "non/existant/path"), zim::EntryNotFound);
}

#if WITH_TEST_DATA
TEST_F(ZimArchive, openRealZimArchive)
{
  const char* const zimfiles[] = {
    "small.zim",
    "wikibooks_be_all_nopic_2017-02.zim",
    "wikibooks_be_all_nopic_2017-02_splitted.zim",
    "wikipedia_en_climate_change_mini_2024-06.zim"
  };

  for ( const std::string fname : zimfiles ) {
    for (auto& testfile: getDataFilePath(fname)) {
      const TestContext ctx{ {"path", testfile.path } };
      std::unique_ptr<zim::Archive> archive;
      EXPECT_NO_THROW( archive.reset(new zim::Archive(testfile.path)) ) << ctx;
      if ( archive ) {
        EXPECT_TRUE( archive->check() ) << ctx;
      }
    }
  }
}

TEST_F(ZimArchive, openSplitZimArchive)
{
  const char* fname = "wikibooks_be_all_nopic_2017-02_splitted.zim";

  for (auto& testfile: getDataFilePath(fname)) {
    const TestContext ctx{ {"path", testfile.path+"aa" } };
    std::unique_ptr<zim::Archive> archive;
    EXPECT_NO_THROW( archive.reset(new zim::Archive(testfile.path+"aa")) ) << ctx;
    if ( archive ) {
      EXPECT_TRUE( archive->check() ) << ctx;
    }
  }
}

struct TestCacheConfig {
  size_t direntCacheSize;
  size_t clusterCacheSize;
  size_t direntLookupCacheSize;
};

struct RefEntry {
  void test_is_equal(const zim::Entry& entry) {
    ASSERT_EQ(path, entry.getPath());
    ASSERT_EQ(title, entry.getTitle());
    ASSERT_EQ(isRedirect, entry.isRedirect());
    if (isRedirect) {
      zim::entry_index_type redirectId = redirect_or_hash;
      ASSERT_EQ(redirectId, entry.getRedirectEntryIndex());
    } else {
      auto hash = std::hash<std::string>{}(std::string(entry.getItem().getData()));
      ASSERT_EQ(redirect_or_hash, hash);
    }
  }

  std::string path;
  std::string title;
  bool        isRedirect;
  // size_t is either 32 or 64 bits and entry_index_type (redirect id) is always 32 bits.
  size_t      redirect_or_hash;
};

struct RefArchiveContent {
  RefArchiveContent(const zim::Archive& archive) {
    for (auto entry:archive.iterEfficient()) {
      RefEntry ref_entry = {
          entry.getPath(),
          entry.getTitle(),
          entry.isRedirect(),
          entry.isRedirect() ? entry.getRedirectEntryIndex() : std::hash<std::string>{}(std::string(entry.getItem().getData())),
      };
      ref_entries.push_back(ref_entry);
    }
  }

  void test_is_equal(const zim::Archive& archive) {
    for (auto ref_entry:ref_entries) {
      auto entry = archive.getEntryByPath(ref_entry.path);
      ref_entry.test_is_equal(entry);
    }
  }
  std::vector<RefEntry> ref_entries;
};


TEST_F(ZimArchive, cacheDontImpactReading)
{
  const TestCacheConfig cacheConfigs[] = {
    {0, 0, 0},
    {1, 1<<20, 1},
    {2, 2<<20, 2},
    {10, 10<<20, 10},
    {1000, 2000<<20, 1000},
    {0, 2000<<20, 1000},
    {1000, 0<<20, 1000},
    {1000, 2000<<20, 0},
    {1, 2000<<20, 1000},
    {1000, 1<<20, 1000},
    {1000, 2000<<20, 1},
  };

  for (auto& testfile: getDataFilePath("small.zim")) {
    RefArchiveContent ref_archive(zim::Archive(testfile.path));

    for (auto cacheConfig: cacheConfigs) {
      auto test_archive = zim::Archive(testfile.path, zim::OpenConfig().preloadDirentRanges(cacheConfig.direntLookupCacheSize));
      test_archive.setDirentCacheMaxSize(cacheConfig.direntCacheSize);
      zim::setClusterCacheMaxSize(cacheConfig.clusterCacheSize);

      EXPECT_EQ(test_archive.getDirentCacheMaxSize(), cacheConfig.direntCacheSize);
      EXPECT_EQ(zim::getClusterCacheMaxSize(), cacheConfig.clusterCacheSize);

      ref_archive.test_is_equal(test_archive);
    }
  }
}

TEST_F(ZimArchive, cacheClean) {
  for (auto& testfile: getDataFilePath("wikibooks_be_all_nopic_2017-02.zim")) {
    EXPECT_EQ(zim::getClusterCacheCurrentSize(), 0); // No clusters in cache
    {
      auto archive = zim::Archive(testfile.path);
      auto range = archive.iterEfficient();
      auto it = range.begin();
      for (auto i = 0; i<50 && it != range.end(); i++, it++) {
        // Be sure to search by path to populate the dirent cache
        auto entry = archive.getEntryByPath(it->getPath());
        auto item = entry.getItem(true);
        auto data = item.getData();
      }
      EXPECT_GT(zim::getClusterCacheCurrentSize(), 0); // No clusters in cache
    }
    EXPECT_EQ(zim::getClusterCacheCurrentSize(), 0); // No clusters in cache
  }
}

TEST_F(ZimArchive, cacheChange)
{
  // We test only one variant here.
  // Each variant has cluster of different size (especially the old "withns" which
  // have a cluster compressed with a algorithm/compression level making input stream
  // having a size of 64MB),
  // this make all the following reasoning about cluster size a bit too complex.
  // As the test here don't test that we can read all variant, we don't have too.
  for (auto& testfile: getDataFilePath("wikibooks_be_all_nopic_2017-02.zim", {"noTitleListingV0"})) {
    // wikibooks has only 2 clusters. One of 492121 bytes and one of 823716 bytes.
    // For a total of 1315837 bytes.
    // Has we try to keep one cluster in the cache, any size under the size of one
    // cluster will not be respected.
    // So we will define 2 limits:
    // 850<<10 : size higher than a cluster size but under 2
    // 2 << 20 : size higher than two clusters
    const size_t L1_SIZE = 850 << 10;
    const size_t L2_SIZE = 2 << 20;

    EXPECT_EQ(zim::getClusterCacheCurrentSize(), 0);
    RefArchiveContent ref_archive(zim::Archive(testfile.path));
    auto archive = zim::Archive(testfile.path);

    archive.setDirentCacheMaxSize(30);
    zim::setClusterCacheMaxSize(L2_SIZE);

    auto ref_it = ref_archive.ref_entries.begin();
    for (auto i = 0; i<50 && ref_it != ref_archive.ref_entries.end(); i++, ref_it++) {
      auto entry = archive.getEntryByPath(ref_it->path);
      ref_it->test_is_equal(entry);
    }
    EXPECT_EQ(archive.getDirentCacheCurrentSize(), 30);
    EXPECT_LE(zim::getClusterCacheCurrentSize(), L2_SIZE); // Only 2 clusters in the file

    // Reduce cache size
    archive.setDirentCacheMaxSize(10);
    zim::setClusterCacheMaxSize(L1_SIZE);

    EXPECT_EQ(archive.getDirentCacheCurrentSize(), 10);
    EXPECT_LE(zim::getClusterCacheCurrentSize(), L1_SIZE);

    // We want to test change of cache while we are iterating on the archive.
    // So we don't reset the ref_it to `range.begin()`.
    for (auto i = 0; i<50 && ref_it != ref_archive.ref_entries.end(); i++, ref_it++) {
      auto entry = archive.getEntryByPath(ref_it->path);
      ref_it->test_is_equal(entry);
    }

    EXPECT_EQ(archive.getDirentCacheCurrentSize(), 10);
    EXPECT_LE(zim::getClusterCacheCurrentSize(), L1_SIZE);

    // Clean cache
    // (More than testing the value, this is needed as we want to be sure the cache is actually populated later)
    archive.setDirentCacheMaxSize(0);
    zim::setClusterCacheMaxSize(0);

    EXPECT_EQ(archive.getDirentCacheCurrentSize(), 0);
    EXPECT_EQ(zim::getClusterCacheCurrentSize(), 0);

    // Increase the cache
    archive.setDirentCacheMaxSize(20);
    zim::setClusterCacheMaxSize(L1_SIZE);
    EXPECT_EQ(archive.getDirentCacheCurrentSize(), 0);
    EXPECT_EQ(zim::getClusterCacheCurrentSize(), 0);

    ref_archive.test_is_equal(archive);
    EXPECT_EQ(archive.getDirentCacheCurrentSize(), 20);
    EXPECT_LE(zim::getClusterCacheCurrentSize(), L1_SIZE);
  }
}


TEST_F(ZimArchive, MultiZimCache)
{
  // Get a list of several zim files to open (whatever the variant)
  std::vector<std::string> zimPaths;
  const char* const zimfiles[] = {
    "wikibooks_be_all_nopic_2017-02.zim",
    "wikibooks_be_all_nopic_2017-02_splitted.zim",
    "wikipedia_en_climate_change_mini_2024-06.zim"
  };

  for ( const std::string fname : zimfiles ) {
    for (auto& testfile: getDataFilePath(fname)) {
      zimPaths.push_back(testfile.path);
    }
  }


  const size_t SMALL_LIMIT = 5 << 20;
  const size_t BIG_LIMIT = 200 << 20;
  zim::setClusterCacheMaxSize(BIG_LIMIT);

  std::vector<zim::Archive> archives;
  for (auto path:zimPaths) {
    auto archive = zim::Archive(path);
    for (auto entry:archive.iterEfficient()) {
      auto item = entry.getItem(true);
      auto data = item.getData();
    }
    archives.push_back(archive);
  }

  EXPECT_LE(zim::getClusterCacheCurrentSize(), BIG_LIMIT);
  zim::setClusterCacheMaxSize(SMALL_LIMIT);
  EXPECT_LE(zim::getClusterCacheCurrentSize(), SMALL_LIMIT);

  // Opening an archive should increase the cluster cache
  zim::setClusterCacheMaxSize(BIG_LIMIT);
  auto current_limit = zim::getClusterCacheCurrentSize();
  {
    auto archive = zim::Archive(zimPaths[0]);
    for (auto entry:archive.iterEfficient()) {
      auto item = entry.getItem(true);
      auto data = item.getData();
    }
    EXPECT_GT(zim::getClusterCacheCurrentSize(), current_limit);
    current_limit = zim::getClusterCacheCurrentSize();
  }
  // Destroying an archive should decrease the cluster cache
  EXPECT_LT(zim::getClusterCacheCurrentSize(), current_limit);

  // Be sure that decreasing the number of archives open also decrease the
  // current cache size, until we reach 0.
  current_limit = zim::getClusterCacheCurrentSize();
  while (!archives.empty()) {
    archives.pop_back();
    EXPECT_LE(zim::getClusterCacheCurrentSize(), current_limit);
    current_limit = zim::getClusterCacheCurrentSize();
  }
  EXPECT_EQ(zim::getClusterCacheCurrentSize(), 0);
}

TEST_F(ZimArchive, openDontFallbackOnNonSplitZimArchive)
{
  const char* fname = "wikibooks_be_all_nopic_2017-02.zim";

  for (auto& testfile: getDataFilePath(fname)) {
    const TestContext ctx{ {"path", testfile.path+"aa" } };
    try {
      zim::Archive(testfile.path+"aa");
      FAIL(); // Exception is expected
    } catch(const std::runtime_error& e) {
      const std::string expected = std::string("Error opening as a split ZIM file: ") + testfile.path + "aa";
      EXPECT_EQ(expected, e.what()) << ctx;
    }
  }
}

TEST_F(ZimArchive, openNonExistantZimArchive)
{
  const std::string fname = "non_existant.zim";

  try {
    zim::Archive archive(fname);
    FAIL(); // Exception is expected
  } catch(const std::runtime_error& e) {
    const std::string expected = std::string("Error opening ZIM file: ") + fname;
    EXPECT_EQ(expected, e.what()) << fname;
  }
}

TEST_F(ZimArchive, openNonExistantZimSplitArchive)
{
  const std::string fname = "non_existant.zimaa";

  try {
    zim::Archive archive(fname);
    FAIL(); // Exception is expected
  } catch(const std::runtime_error& e) {
    const std::string expected = std::string("Error opening as a split ZIM file: ") + fname;
    EXPECT_EQ(expected, e.what()) << fname;
  }
}

TEST_F(ZimArchive, randomEntry)
{
  const char* const zimfiles[] = {
    "wikibooks_be_all_nopic_2017-02.zim",
    "wikibooks_be_all_nopic_2017-02_splitted.zim",
    "wikipedia_en_climate_change_mini_2024-06.zim"
  };

  for ( const std::string fname : zimfiles ) {
    for (auto& testfile: getDataFilePath(fname)) {
      const TestContext ctx{ {"path", testfile.path } };
      const zim::Archive archive(testfile.path);
      try {
        auto randomEntry = archive.getRandomEntry();
        const auto item = randomEntry.getItem(true);
        ASSERT_TRUE(item.getMimetype().find("text/html") != std::string::npos) << ctx;
      } catch (zim::EntryNotFound& e) {
        FAIL() << "Impossible to find a random Entry in " << fname << ".\n"
               << "This may occur even if this is not a bug (random will be random).\n"
               << "Please re-run the tests.";
      }
    }
  }
}

TEST_F(ZimArchive, illustration)
{
  const char* const zimfiles[] = {
    "small.zim",
    "wikibooks_be_all_nopic_2017-02.zim"
  };

  for ( const std::string fname : zimfiles ) {
    for (auto& testfile: getDataFilePath(fname)) {
      const TestContext ctx{ {"path", testfile.path } };
      const zim::Archive archive(testfile.path);
      ASSERT_TRUE(archive.hasIllustration(48)) << ctx;
      auto illustrationItem = archive.getIllustrationItem(48);
      if(testfile.category == "withns") {
        ASSERT_EQ(illustrationItem.getPath(), "I/favicon.png") << ctx;
      } else {
        ASSERT_EQ(illustrationItem.getPath(), "Illustration_48x48@1") << ctx;
      }
      ASSERT_EQ(archive.getIllustrationInfos(), zim::Archive::IllustrationInfos({
                  {48, 48, 1.0, {}},
      }));
    }
  }
}

struct ZimFileInfo {
  zim::entry_index_type articleCount, entryCount, allEntryCount;
};

struct TestDataInfo {
  const char* const name;
  zim::entry_index_type mediaCount;
  ZimFileInfo withnsInfo, nonsInfo, noTitleListingV0Info;


  const ZimFileInfo& getZimFileInfo(const std::string& category) const {
    if (category == "nons") {
      return nonsInfo;
    } else if (category == "withns") {
      return withnsInfo;
    } else if (category == "noTitleListingV0") {
      return noTitleListingV0Info;
    }
    throw std::runtime_error("Unknown category");
  }
};

TEST_F(ZimArchive, articleNumber)
{
  TestDataInfo zimfiles[] = {
     // Name                                          mediaCount,  withns                               nons                                 noTitleListingV0
     //                                                            {articles, userEntries, allEntries}, {articles, userEntries, allEntries}, {articles, userEntries, allEntries}
    {"small.zim",                                     1,           { 1,       17,          17,       }, { 1,       2,           16        }, { 1,       2,           16        }},
// For some unknown reason, nons wikibooks already doesn't contain a v0 title index so number of allEntries is equal to noTitleListingV0.
// But header titlePtrPos is initialized in nons and is 0 in noTitleListingV0.
// I suspect here that nons file was generated using a local dev buggy zimrecreate.
    {"wikibooks_be_all_nopic_2017-02.zim",            34,          { 66,      118,         118,      }, { 66,      109,         123       }, { 66,      109,         123       }},
    {"wikibooks_be_all_nopic_2017-02_splitted.zim",   34,          { 66,      118,         118,      }, { 66,      109,         123       }, { 66,      109,         123       }},
    {"wikipedia_en_climate_change_mini_2024-06.zim",  111,         { 3821,    20565,       20565,    }, { 3821,    20551,       20568     }, { 3821,    20551,       20567     }}
  };
  // "withns" zim files have no notion of user entries, so EntryCount == allEntryCount.
  // for small.zim, there is always 1 article, whatever the article is in 'A' namespace or in specific index.

  for ( const auto& testdata : zimfiles ) {
    for (auto& testfile: getDataFilePath(testdata.name)) {
      const TestContext ctx{ {"path", testfile.path } };
      const auto& testZimInfo = testdata.getZimFileInfo(testfile.category);
      const zim::Archive archive(testfile.path);
      EXPECT_EQ( archive.getAllEntryCount(), testZimInfo.allEntryCount ) << ctx;
      EXPECT_EQ( archive.getEntryCount(), testZimInfo.entryCount ) << ctx;
      EXPECT_EQ( archive.getArticleCount(), testZimInfo.articleCount ) << ctx;
      EXPECT_EQ( archive.getMediaCount(), testdata.mediaCount) << ctx;
    }
  }
}
#endif

class CapturedStderr
{
  std::ostringstream buffer;
  std::streambuf* const sbuf;
public:
  CapturedStderr()
    : sbuf(std::cerr.rdbuf())
  {
    std::cerr.rdbuf(buffer.rdbuf());
  }

  CapturedStderr(const CapturedStderr&) = delete;

  ~CapturedStderr()
  {
    std::cerr.rdbuf(sbuf);
  }

  operator std::string() const { return buffer.str(); }
};

#define EXPECT_BROKEN_ZIMFILE(ZIMPATH, EXPECTED_STDERROR_TEXT) \
  CapturedStderr stderror;                                     \
  EXPECT_FALSE(zim::validate(ZIMPATH, checksToRun));           \
  EXPECT_EQ(EXPECTED_STDERROR_TEXT, std::string(stderror)) << ZIMPATH; \
  ASSERT_EQ(zim::getClusterCacheCurrentSize(), 0);

#define TEST_BROKEN_ZIM_NAME(ZIMNAME, EXPECTED)                \
for(auto& testfile: getDataFilePath(ZIMNAME)) {EXPECT_BROKEN_ZIMFILE(testfile.path, EXPECTED)}

#define TEST_BROKEN_ZIM_NAME_CAT(ZIMNAME, CAT, EXPECTED)                \
for(auto& testfile: getDataFilePath(ZIMNAME, CAT)) {EXPECT_BROKEN_ZIMFILE(testfile.path, EXPECTED)}

// Use an intermediate define to avoid CPP to interpreted the comma as a parameter separator.
#define WITH_TITLE_IDX_CAT {"withns", "nons"}

#if WITH_TEST_DATA
TEST_F(ZimArchive, validate)
{
  zim::IntegrityCheckList all;
  all.set();

  for(auto& testfile: getDataFilePath("small.zim")) {
    ASSERT_TRUE(zim::validate(testfile.path, all));
  }

  zim::IntegrityCheckList checksToRun;
  checksToRun.set();
  checksToRun.reset(size_t(zim::IntegrityCheck::CHECKSUM));

  TEST_BROKEN_ZIM_NAME(
    "invalid.smaller_than_header.zim",
    "zim-file is too small to contain a header\n"
  );

  TEST_BROKEN_ZIM_NAME(
    "invalid.outofbounds_urlptrpos.zim",
    "Dirent pointer table outside (or not fully inside) ZIM file.\n"
  );

  for(auto& testfile: getDataFilePath("invalid.outofbounds_titleptrpos.zim")) {
    std::string expected;
    if (testfile.category == "withns") {
      expected = "Title index table outside (or not fully inside) ZIM file.\n";
    } else if ( testfile.category == "nons") {
      expected = "Full Title index table outside (or not fully inside) ZIM file.\n";
    } else {
      continue;
    }
    EXPECT_BROKEN_ZIMFILE(testfile.path, expected)
  }

  TEST_BROKEN_ZIM_NAME(
    "invalid.outofbounds_clusterptrpos.zim",
    "Cluster pointer table outside (or not fully inside) ZIM file.\n"
  );

  TEST_BROKEN_ZIM_NAME(
    "invalid.invalid_mimelistpos.zim",
    "mimelistPos must be 80.\n"
  );

  TEST_BROKEN_ZIM_NAME(
    "invalid.invalid_checksumpos.zim",
    "Zim file(s) is of bad size or corrupted.\n"
  );

  TEST_BROKEN_ZIM_NAME(
    "invalid.outofbounds_first_direntptr.zim",
    "Invalid dirent pointer\n"
  );

  TEST_BROKEN_ZIM_NAME(
    "invalid.outofbounds_last_direntptr.zim",
    "Invalid dirent pointer\n"
  );

  TEST_BROKEN_ZIM_NAME_CAT(
    "invalid.outofbounds_first_title_entry.zim",
    WITH_TITLE_IDX_CAT,
    "Invalid title index entry.\n"
  );

  TEST_BROKEN_ZIM_NAME_CAT(
    "invalid.outofbounds_last_title_entry.zim",
    WITH_TITLE_IDX_CAT,
    "Invalid title index entry.\n"
  );

  TEST_BROKEN_ZIM_NAME(
    "invalid.outofbounds_first_clusterptr.zim",
    "Invalid cluster pointer\n"
  );

  TEST_BROKEN_ZIM_NAME(
    "invalid.too_small_offset_of_first_blob_in_cluster_0.zim",
     "Error parsing cluster. Offset of the first blob is too small.\n"
  )

  TEST_BROKEN_ZIM_NAME(
    "invalid.too_small_offset_of_first_blob_in_cluster_4.zim",
     "Error parsing cluster. Offset of the first blob is too small.\n"
  )

  TEST_BROKEN_ZIM_NAME(
    "invalid.too_small_offset_of_first_blob_in_cluster_7.zim",
     "Error parsing cluster. Offset of the first blob is too small.\n"
  )

  TEST_BROKEN_ZIM_NAME(
    "invalid.misaligned_offset_of_first_blob_in_cluster_9.zim",
     "Error parsing cluster. Offset of the first blob is not properly aligned.\n"
  )

  TEST_BROKEN_ZIM_NAME(
    "invalid.misaligned_offset_of_first_blob_in_cluster_10.zim",
     "Error parsing cluster. Offset of the first blob is not properly aligned.\n"
  )

  TEST_BROKEN_ZIM_NAME(
    "invalid.misaligned_offset_of_first_blob_in_cluster_11.zim",
     "Error parsing cluster. Offset of the first blob is not properly aligned.\n"
  )

  TEST_BROKEN_ZIM_NAME(
    "invalid.offset_in_cluster.zim",
     "Error parsing cluster. Offsets are not ordered.\n"
  )


  for(auto& testfile: getDataFilePath("invalid.nonsorted_dirent_table.zim")) {
    std::string expected;
    if (testfile.category == "withns") {
      expected = "Dirent table is not properly sorted:\n"
                 "  #0: A/main.html\n"
                 "  #1: -/favicon\n";
    } else {
      expected = "Dirent table is not properly sorted:\n"
                 "  #0: C/main.html\n"
                 "  #1: C/favicon.png\n";
    }
    EXPECT_BROKEN_ZIMFILE(testfile.path, expected)
  }

  TEST_BROKEN_ZIM_NAME_CAT(
    "invalid.nonsorted_title_index.zim",
    WITH_TITLE_IDX_CAT,
    "Title index is not properly sorted.\n"
  );

  TEST_BROKEN_ZIM_NAME(
    "invalid.bad_mimetype_list.zim",
    "Error getting mimelists.\n"
  );

  for(auto& testfile: getDataFilePath("invalid.bad_mimetype_in_dirent.zim")) {
    std::string expected;
    if (testfile.category == "withns") {
      expected = "Entry M/Language has invalid MIME-type value 1234.\n";
    } else if (testfile.category == "nons") {
      expected = "Entry M/Publisher has invalid MIME-type value 1234.\n";
    } else {
      expected = "Entry M/Name has invalid MIME-type value 1234.\n";
    }
    EXPECT_BROKEN_ZIMFILE(testfile.path, expected)
  }
}


void checkEquivalence(const zim::Archive& archive1, const zim::Archive& archive2)
{
  EXPECT_EQ(archive1.getFilesize(), archive2.getFilesize());
  EXPECT_EQ(archive1.getClusterCount(), archive2.getClusterCount());

  ASSERT_EQ(archive1.getEntryCount(), archive2.getEntryCount());
  const zim::Entry mainEntry = archive1.getMainEntry();
  ASSERT_EQ(mainEntry.getTitle(), archive2.getMainEntry().getTitle());

  ASSERT_NE(0U, archive1.getEntryCount()); // ==> below loop is not a noop
  {
    auto range1 = archive1.iterEfficient();
    auto range2 = archive2.iterEfficient();
    for ( auto it1=range1.begin(), it2=range2.begin(); it1!=range1.end() && it2!=range2.end(); ++it1, ++it2 ) {
      auto& entry1 = *it1;
      auto& entry2 = *it2;
      ASSERT_EQ(entry1.getIndex(), entry2.getIndex());
      ASSERT_EQ(entry1.getPath(), entry2.getPath());
      ASSERT_EQ(entry1.getTitle(), entry2.getTitle());
      ASSERT_EQ(entry1.isRedirect(), entry2.isRedirect());
      if (!entry1.isRedirect()) {
        auto item1 = entry1.getItem();
        auto item2 = entry2.getItem();
        ASSERT_EQ(item1.getMimetype(), item2.getMimetype());
        ASSERT_EQ(item1.getSize(), item2.getSize());
        ASSERT_EQ(item1.getData(), item2.getData());
      }
    }
  }

  {
    auto range1 = archive1.iterByPath();
    auto range2 = archive2.iterByPath();
    for ( auto it1=range1.begin(), it2=range2.begin(); it1!=range1.end() && it2!=range2.end(); ++it1, ++it2 ) {
      auto& entry1 = *it1;
      auto& entry2 = *it2;

      ASSERT_EQ(entry1.getIndex(), entry2.getIndex());
    }
  }

  {
    auto range1 = archive1.iterByTitle();
    auto range2 = archive2.iterByTitle();
    for ( auto it1=range1.begin(), it2=range2.begin(); it1!=range1.end() && it2!=range2.end(); ++it1, ++it2 ) {
      auto& entry1 = *it1;
      auto& entry2 = *it2;

      ASSERT_EQ(entry1.getIndex(), entry2.getIndex());
    }
  }

#if defined(ENABLE_XAPIAN)
  if ( archive1.hasTitleIndex() )
  {
    // Resolve any potential redirect.
    auto mainItem = mainEntry.getItem(true);
    zim::SuggestionSearcher searcher1(archive1);
    zim::SuggestionSearcher searcher2(archive2);
    std::string query = mainItem.getTitle();
    auto search1 = searcher1.suggest(query);
    auto search2 = searcher2.suggest(query);
    ASSERT_NE(0, search1.getEstimatedMatches());
    ASSERT_EQ(search1.getEstimatedMatches(), search2.getEstimatedMatches());

    auto result1 = search1.getResults(0, archive1.getEntryCount());
    auto result2 = search2.getResults(0, archive2.getEntryCount());
    auto firstSearchItem1 = result1.begin().getEntry().getItem(true);
    auto firstSearchItem2 = result2.begin().getEntry().getItem(true);
    ASSERT_EQ(mainItem.getPath(), firstSearchItem1.getPath());
    ASSERT_EQ(mainItem.getPath(), firstSearchItem2.getPath());
    ASSERT_EQ(result1.size(), result2.size());
  }
#endif
}

TEST_F(ZimArchive, multipart)
{
  auto nonSplittedZims = getDataFilePath("wikibooks_be_all_nopic_2017-02.zim");
  auto splittedZims = getDataFilePath("wikibooks_be_all_nopic_2017-02_splitted.zim");

  ASSERT_EQ(nonSplittedZims.size(), splittedZims.size()) << "We must have same number of zim files. (This is a test data issue)";
  for(auto i=0UL; i < nonSplittedZims.size(); i++) {
    const zim::Archive archive1(nonSplittedZims[i].path);
    const zim::Archive archive2(splittedZims[i].path);
    ASSERT_FALSE(archive1.isMultiPart());
    ASSERT_TRUE (archive2.isMultiPart());

    checkEquivalence(archive1, archive2);
  }
}

#ifdef _WIN32
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#undef min
#undef max
# define OPEN_READ_ONLY(path) _open((path).c_str(), _O_RDONLY)
#else
# define OPEN_READ_ONLY(path) open((path).c_str(), O_RDONLY)
#endif

#ifndef _WIN32
TEST_F(ZimArchive, openByFD)
{
  for(auto& testfile: getDataFilePath("small.zim")) {
    const zim::Archive archive1(testfile.path);
    const int fd = OPEN_READ_ONLY(testfile.path);
    const zim::Archive archive2(fd);

    checkEquivalence(archive1, archive2);
  }
}

TEST_F(ZimArchive, openZIMFileEmbeddedInAnotherFile)
{
  auto normalZims = getDataFilePath("small.zim");
  auto embeddedZims = getDataFilePath("small.zim.embedded");

  ASSERT_EQ(normalZims.size(), embeddedZims.size()) << "We must have same number of zim files. (This is a test data issue)";
  for(auto i=0UL; i < normalZims.size(); i++) {
    const zim::Archive archive1(normalZims[i].path);
    const int fd = OPEN_READ_ONLY(embeddedZims[i].path);
    const zim::Archive archive2(zim::FdInput(fd, 8, archive1.getFilesize()));
    close(fd);

    checkEquivalence(archive1, archive2);
  }
}

TEST_F(ZimArchive, openZIMFileMultiPartEmbeddedInAnotherFile)
{
  auto normalZims = getDataFilePath("small.zim");
  auto embeddedZims = getDataFilePath("small.zim.embedded.multi");

  ASSERT_EQ(normalZims.size(), embeddedZims.size()) << "We must have same number of zim files. (This is a test data issue)";
  for(auto i=0UL; i < normalZims.size(); i++) {
    const zim::Archive archive1(normalZims[i].path);
    auto archive_size = archive1.getFilesize();

    std::vector<zim::FdInput> fds;
    zim::offset_type start_offset = std::string("BEGINZIMMULTIPART").size();
    while (archive_size > 2048) {
      int fd = OPEN_READ_ONLY(embeddedZims[i].path);
      fds.push_back(zim::FdInput(fd, start_offset, 2048));
      start_offset += 2048 + std::string("NEWSECTIONZIMMULTI").size();
      archive_size -= 2048;
    }
    int fd = OPEN_READ_ONLY(embeddedZims[i].path);
    fds.push_back(zim::FdInput(fd, start_offset, archive_size));

    const zim::Archive archive2(fds);

    for(auto &fd: fds) {
      close(fd.fd);
    }

    checkEquivalence(archive1, archive2);
  }
}
#endif // not _WIN32
#endif // WITH_TEST_DATA


#if WITH_TEST_DATA
zim::Blob readItemData(const zim::ItemDataDirectAccessInfo& dai, zim::size_type size)
{
  zim::DEFAULTFS::FD fd(zim::DEFAULTFS::openFile(dai.filename));
  std::shared_ptr<char> data(new char[size], std::default_delete<char[]>());
  fd.readAt(data.get(), zim::zsize_t(size), zim::offset_t(dai.offset));
  return zim::Blob(data, size);
}

TEST_F(ZimArchive, getDirectAccessInformation)
{
  for(auto& testfile:getDataFilePath("small.zim")) {
    const zim::Archive archive(testfile.path);
    zim::entry_index_type checkedItemCount = 0;
    for ( auto entry : archive.iterEfficient() ) {
      if (!entry.isRedirect()) {
        const TestContext ctx{ {"entry", entry.getPath() } };
        const auto item = entry.getItem();
        const auto dai = item.getDirectAccessInformation();
        if ( dai.isValid() ) {
          ++checkedItemCount;
          EXPECT_EQ(item.getData(), readItemData(dai, item.getSize())) << ctx;
        }
      }
    }
    ASSERT_NE(0U, checkedItemCount);
  }
}

#ifndef _WIN32
TEST_F(ZimArchive, getDirectAccessInformationInAnArchiveOpenedByFD)
{
  for(auto& testfile:getDataFilePath("small.zim")) {
    const int fd = OPEN_READ_ONLY(testfile.path);
    const zim::Archive archive(fd);
    zim::entry_index_type checkedItemCount = 0;
    for ( auto entry : archive.iterEfficient() ) {
      if (!entry.isRedirect()) {
        const TestContext ctx{ {"entry", entry.getPath() } };
        const auto item = entry.getItem();
        const auto dai = item.getDirectAccessInformation();
        if ( dai.isValid() ) {
          ++checkedItemCount;
          EXPECT_EQ(item.getData(), readItemData(dai, item.getSize())) << ctx;
        }
      }
    }
    ASSERT_NE(0U, checkedItemCount);
  }
}

TEST_F(ZimArchive, getDirectAccessInformationFromEmbeddedArchive)
{
  auto normalZims = getDataFilePath("small.zim");
  auto embeddedZims = getDataFilePath("small.zim.embedded");

  ASSERT_EQ(normalZims.size(), embeddedZims.size()) << "We must have same number of zim files. (This is a test data issue)";
  for(auto i=0UL; i < normalZims.size(); i++) {
    const int fd = OPEN_READ_ONLY(embeddedZims[i].path);
    const auto size = zim::DEFAULTFS::openFile(normalZims[i].path).getSize();
    const zim::Archive archive(zim::FdInput(fd, 8, size.v));
    zim::entry_index_type checkedItemCount = 0;
    for ( auto entry : archive.iterEfficient() ) {
      if (!entry.isRedirect()) {
        const TestContext ctx{ {"entry", entry.getPath() } };
        const auto item = entry.getItem();
        const auto dai = item.getDirectAccessInformation();
        if ( dai.isValid() ) {
          ++checkedItemCount;
          EXPECT_EQ(item.getData(), readItemData(dai, item.getSize())) << ctx;
        }
      }
    }
    ASSERT_NE(0U, checkedItemCount);
  }
}
#endif // not _WIN32
#endif // WITH_TEST_DATA

} // unnamed namespace
