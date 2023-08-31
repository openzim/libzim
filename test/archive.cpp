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
#include <zim/item.h>
#include <zim/search.h>
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

TEST(ZimArchive, openingAnInvalidZimArchiveFails)
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

TEST(ZimArchive, openingAnEmptyZimArchiveSucceeds)
{
  const auto tmpfile = makeTempFile("empty_zim_file", emptyZimArchiveContent());

  zim::Archive archive(tmpfile->path());
  ASSERT_TRUE(archive.check());
}

bool isNastyOffset(int offset) {
  if ( 6 <= offset && offset < 24 ) // Minor version or uuid
    return false;

  if ( 64 <= offset && offset < 72 ) // page or layout index
    return false;

  return true;
}

TEST(ZimArchive, nastyEmptyZimArchive)
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

TEST(ZimArchive, wrongChecksumInEmptyZimArchive)
{
  std::string zimfileContent = emptyZimArchiveContent();
  zimfileContent[85] = '\xff';
  const auto tmpfile = makeTempFile("wrong_checksum_empty_zim_file", zimfileContent);

  zim::Archive archive(tmpfile->path());
  ASSERT_FALSE(archive.check());
}


TEST(ZimArchive, openCreatedArchive)
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
  creator.addIllustration(48, "PNGBinaryContent48");
  creator.addIllustration(96, "PNGBinaryContent96");
  creator.setMainPath("foo");
  creator.addRedirection("foo3", "FooRedirection", "foo"); // No a front article.
  creator.addRedirection("foo4", "FooRedirection", "NoExistant"); // Invalid redirection, must be removed by creator
  creator.finishZimCreation();

  zim::Archive archive(tempPath);
#if !defined(ENABLE_XAPIAN)
// 2*listingIndex + M/Counter + M/Title + mainpage + 2*Illustration + 2*Item + redirection
#define ALL_ENTRY_COUNT 10U
#else
// same as above + 2 xapian indexes.
#define ALL_ENTRY_COUNT 12U
#endif
  ASSERT_EQ(archive.getAllEntryCount(), ALL_ENTRY_COUNT);
#undef ALL_ENTRY_COUNT
  ASSERT_EQ(archive.getEntryCount(), 3U);
  ASSERT_EQ(archive.getArticleCount(), 1U);
  ASSERT_EQ(archive.getUuid(), uuid);
  ASSERT_EQ(archive.getMetadataKeys(), std::vector<std::string>({"Counter", "Illustration_48x48@1", "Illustration_96x96@1", "Title"}));
  ASSERT_EQ(archive.getIllustrationSizes(), std::set<unsigned int>({48, 96}));
  ASSERT_TRUE(archive.hasMainEntry());

  ASSERT_EQ(archive.getMetadata("Title"), "This is a title");
  auto titleMeta = archive.getMetadataItem("Title");
  ASSERT_EQ(std::string(titleMeta.getData()), "This is a title");
  ASSERT_EQ(titleMeta.getMimetype(), "text/plain;charset=utf-8");
  ASSERT_EQ(archive.getMetadata("Counter"), "text/html=2");
  auto illu48 = archive.getIllustrationItem(48);
  ASSERT_EQ(illu48.getPath(), "Illustration_48x48@1");
  ASSERT_EQ(std::string(illu48.getData()), "PNGBinaryContent48");
  auto illu48Meta = archive.getMetadataItem(illu48.getPath());
  ASSERT_EQ(std::string(illu48Meta.getData()), "PNGBinaryContent48");
  ASSERT_EQ(illu48Meta.getMimetype(), "image/png");
  auto illu96 = archive.getIllustrationItem(96);
  ASSERT_EQ(illu96.getPath(), "Illustration_96x96@1");
  ASSERT_EQ(std::string(illu96.getData()), "PNGBinaryContent96");

  auto foo = archive.getEntryByPath("foo");
  ASSERT_EQ(foo.getPath(), "foo");
  ASSERT_EQ(foo.getTitle(), "Foo");
  ASSERT_EQ(std::string(foo.getItem().getData()), "FooContent");
  ASSERT_THROW(foo.getRedirectEntry(), zim::InvalidType);
  ASSERT_THROW(foo.getRedirectEntryIndex(), zim::InvalidType);

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
}

#if WITH_TEST_DATA
TEST(ZimArchive, openRealZimArchive)
{
  const char* const zimfiles[] = {
    "small.zim",
    "wikibooks_be_all_nopic_2017-02.zim",
    "wikibooks_be_all_nopic_2017-02_splitted.zim",
    "wikipedia_en_climate_change_nopic_2020-01.zim"
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

TEST(ZimArchive, randomEntry)
{
  const char* const zimfiles[] = {
    "wikibooks_be_all_nopic_2017-02.zim",
    "wikibooks_be_all_nopic_2017-02_splitted.zim",
    "wikipedia_en_climate_change_nopic_2020-01.zim"
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

TEST(ZimArchive, illustration)
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
      if(testfile.category == "nons") {
        ASSERT_EQ(illustrationItem.getPath(), "Illustration_48x48@1") << ctx;
      } else {
        ASSERT_EQ(illustrationItem.getPath(), "I/favicon.png") << ctx;
      }
      ASSERT_EQ(archive.getIllustrationSizes(), std::set<unsigned int>({48}));
    }
  }
}

struct ZimFileInfo {
  zim::entry_index_type articleCount, entryCount, allEntryCount;
};

struct TestDataInfo {
  const char* const name;
  zim::entry_index_type mediaCount;
  ZimFileInfo withnsInfo, nonsInfo;


  const ZimFileInfo& getZimFileInfo(const std::string& category) const {
    if (category == "nons") {
      return nonsInfo;
    } else if (category == "withns") {
      return withnsInfo;
    }
    throw std::runtime_error("Unknown category");
  }
};

TEST(ZimArchive, articleNumber)
{
  TestDataInfo zimfiles[] = {
     // Name                                          mediaCount,  withns                                           nons
     //                                                            {articles, userEntries, allEntries}, {articles, userEntries, allEntries}
    {"small.zim",                                     1,           { 1,       17,          17,       }, { 1,       2,           16        }},
    {"wikibooks_be_all_nopic_2017-02.zim",            34,          { 66,      118,         118,      }, { 66,      109,         123       }},
    {"wikibooks_be_all_nopic_2017-02_splitted.zim",   34,          { 66,      118,         118,      }, { 66,      109,         123       }},
    {"wikipedia_en_climate_change_nopic_2020-01.zim", 333,         { 1837,    7646,        7646,     }, { 1837,    7633,        7649      }}
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
  EXPECT_EQ(EXPECTED_STDERROR_TEXT, std::string(stderror)) << ZIMPATH;

#define TEST_BROKEN_ZIM_NAME(ZIMNAME, EXPECTED)                \
for(auto& testfile: getDataFilePath(ZIMNAME)) {EXPECT_BROKEN_ZIMFILE(testfile.path, EXPECTED)}

#if WITH_TEST_DATA
TEST(ZimArchive, validate)
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
    } else {
      expected = "Full Title index table outside (or not fully inside) ZIM file.\n";
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
    "Checksum position is not valid\n"
  );

  TEST_BROKEN_ZIM_NAME(
    "invalid.outofbounds_first_direntptr.zim",
    "Invalid dirent pointer\n"
  );

  TEST_BROKEN_ZIM_NAME(
    "invalid.outofbounds_last_direntptr.zim",
    "Invalid dirent pointer\n"
  );

  TEST_BROKEN_ZIM_NAME(
    "invalid.outofbounds_first_title_entry.zim",
    "Invalid title index entry.\n"
  );

  TEST_BROKEN_ZIM_NAME(
    "invalid.outofbounds_last_title_entry.zim",
    "Invalid title index entry.\n"
  );

  TEST_BROKEN_ZIM_NAME(
    "invalid.outofbounds_first_clusterptr.zim",
    "Invalid cluster pointer\n"
  );


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

  TEST_BROKEN_ZIM_NAME(
    "invalid.nonsorted_title_index.zim",
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
    } else {
      expected = "Entry M/Publisher has invalid MIME-type value 1234.\n";
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

TEST(ZimArchive, multipart)
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
TEST(ZimArchive, openByFD)
{
  for(auto& testfile: getDataFilePath("small.zim")) {
    const zim::Archive archive1(testfile.path);
    const int fd = OPEN_READ_ONLY(testfile.path);
    const zim::Archive archive2(fd);

    checkEquivalence(archive1, archive2);
  }
}

TEST(ZimArchive, openZIMFileEmbeddedInAnotherFile)
{
  auto normalZims = getDataFilePath("small.zim");
  auto embeddedZims = getDataFilePath("small.zim.embedded");

  ASSERT_EQ(normalZims.size(), embeddedZims.size()) << "We must have same number of zim files. (This is a test data issue)";
  for(auto i=0UL; i < normalZims.size(); i++) {
    const zim::Archive archive1(normalZims[i].path);
    const int fd = OPEN_READ_ONLY(embeddedZims[i].path);
    const zim::Archive archive2(fd, 8, archive1.getFilesize());

    checkEquivalence(archive1, archive2);
  }
}
#endif // not _WIN32
#endif // WITH_TEST_DATA


#if WITH_TEST_DATA
zim::Blob readItemData(const zim::Item::DirectAccessInfo& dai, zim::size_type size)
{
  zim::DEFAULTFS::FD fd(zim::DEFAULTFS::openFile(dai.first));
  std::shared_ptr<char> data(new char[size]);
  fd.readAt(data.get(), zim::zsize_t(size), zim::offset_t(dai.second));
  return zim::Blob(data, size);
}

TEST(ZimArchive, getDirectAccessInformation)
{
  for(auto& testfile:getDataFilePath("small.zim")) {
    const zim::Archive archive(testfile.path);
    zim::entry_index_type checkedItemCount = 0;
    for ( auto entry : archive.iterEfficient() ) {
      if (!entry.isRedirect()) {
        const TestContext ctx{ {"entry", entry.getPath() } };
        const auto item = entry.getItem();
        const auto dai = item.getDirectAccessInformation();
        if ( dai.first != "" ) {
          ++checkedItemCount;
          EXPECT_EQ(item.getData(), readItemData(dai, item.getSize())) << ctx;
        }
      }
    }
    ASSERT_NE(0U, checkedItemCount);
  }
}

#ifndef _WIN32
TEST(ZimArchive, getDirectAccessInformationInAnArchiveOpenedByFD)
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
        if ( dai.first != "" ) {
          ++checkedItemCount;
          EXPECT_EQ(item.getData(), readItemData(dai, item.getSize())) << ctx;
        }
      }
    }
    ASSERT_NE(0U, checkedItemCount);
  }
}

TEST(ZimArchive, getDirectAccessInformationFromEmbeddedArchive)
{
  auto normalZims = getDataFilePath("small.zim");
  auto embeddedZims = getDataFilePath("small.zim.embedded");

  ASSERT_EQ(normalZims.size(), embeddedZims.size()) << "We must have same number of zim files. (This is a test data issue)";
  for(auto i=0UL; i < normalZims.size(); i++) {
    const int fd = OPEN_READ_ONLY(embeddedZims[i].path);
    const auto size = zim::DEFAULTFS::openFile(normalZims[i].path).getSize();
    const zim::Archive archive(fd, 8, size.v);
    zim::entry_index_type checkedItemCount = 0;
    for ( auto entry : archive.iterEfficient() ) {
      if (!entry.isRedirect()) {
        const TestContext ctx{ {"entry", entry.getPath() } };
        const auto item = entry.getItem();
        const auto dai = item.getDirectAccessInformation();
        if ( dai.first != "" ) {
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
