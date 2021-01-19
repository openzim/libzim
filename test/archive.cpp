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

#include "tools.h"
#include "../src/fs.h"

#include "gtest/gtest.h"

namespace
{

using zim::unittests::TempFile;

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

std::unique_ptr<TempFile>
makeTempFile(const char* name, const std::string& content)
{
  std::unique_ptr<TempFile> p(new TempFile(name));
  write(p->fd(), &content[0], content.size());
  p->close();
  return p;
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

TEST(ZimArchive, openRealZimArchive)
{
  const char* const zimfiles[] = {
    "small.zim",
    "wikibooks_be_all_nopic_2017-02.zim",
    "wikibooks_be_all_nopic_2017-02_splitted.zim",
    "wikipedia_en_climate_change_nopic_2020-01.zim"
  };

  for ( const std::string fname : zimfiles ) {
    const std::string path = zim::DEFAULTFS::join("data", fname);
    const TestContext ctx{ {"path", path } };
    std::unique_ptr<zim::Archive> archive;
    EXPECT_NO_THROW( archive.reset(new zim::Archive(path)) ) << ctx;
    if ( archive ) {
      EXPECT_TRUE( archive->check() ) << ctx;
    }
  }
}

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

#define EXPECT_BROKEN_ZIMFILE(zimpath, expected_stderror_text)    \
  {                                                               \
    zim::IntegrityCheckList checksToRun;                          \
    checksToRun.set();                                            \
    checksToRun.reset(size_t(zim::IntegrityCheck::CHECKSUM));     \
    CapturedStderr stderror;                                      \
    EXPECT_FALSE(zim::validate(zimpath, checksToRun));            \
    EXPECT_EQ(expected_stderror_text, std::string(stderror));     \
  }

TEST(ZimArchive, validate)
{
  zim::IntegrityCheckList all;
  all.set();

  ASSERT_TRUE(zim::validate("./data/small.zim", all));

  EXPECT_BROKEN_ZIMFILE(
    "./data/invalid.smaller_than_header.zim",
    "zim-file is too small to contain a header\n"
  );

  EXPECT_BROKEN_ZIMFILE(
    "./data/invalid.outofbounds_urlptrpos.zim",
    "Dirent pointer table outside (or not fully inside) ZIM file.\n"
  );

  EXPECT_BROKEN_ZIMFILE(
    "./data/invalid.outofbounds_titleptrpos.zim",
    "Title index table outside (or not fully inside) ZIM file.\n"
  );

  EXPECT_BROKEN_ZIMFILE(
    "./data/invalid.outofbounds_clusterptrpos.zim",
    "Cluster pointer table outside (or not fully inside) ZIM file.\n"
  );

  EXPECT_BROKEN_ZIMFILE(
    "./data/invalid.invalid_mimelistpos.zim",
    "mimelistPos must be 80.\n"
  );

  EXPECT_BROKEN_ZIMFILE(
    "./data/invalid.invalid_checksumpos.zim",
    "Checksum position is not valid\n"
  );

  EXPECT_BROKEN_ZIMFILE(
    "./data/invalid.outofbounds_first_direntptr.zim",
    "Invalid dirent pointer\n"
  );

  EXPECT_BROKEN_ZIMFILE(
    "./data/invalid.outofbounds_last_direntptr.zim",
    "Invalid dirent pointer\n"
  );

  EXPECT_BROKEN_ZIMFILE(
    "./data/invalid.outofbounds_first_title_entry.zim",
    "Invalid title index entry\n"
  );

  EXPECT_BROKEN_ZIMFILE(
    "./data/invalid.outofbounds_last_title_entry.zim",
    "Invalid title index entry\n"
  );

  EXPECT_BROKEN_ZIMFILE(
    "./data/invalid.outofbounds_first_clusterptr.zim",
    "Invalid cluster pointer\n"
  );

  EXPECT_BROKEN_ZIMFILE(
    "./data/invalid.nonsorted_dirent_table.zim",
    "Dirent table is not properly sorted:\n"
    "  #0: A/main.html\n"
    "  #1: -/favicon\n"
  );

  EXPECT_BROKEN_ZIMFILE(
    "./data/invalid.nonsorted_title_index.zim",
    "Title index is not properly sorted:\n"
    "  #0: A/Test ZIM file\n"
    "  #1: -/favicon\n"
  );

  EXPECT_BROKEN_ZIMFILE(
    "./data/invalid.bad_mimetype_list.zim",
    "Error getting mimelists.\n"
  );
}

void checkEquivalence(const zim::Archive& archive1, const zim::Archive& archive2)
{
  EXPECT_EQ(archive1.getFilesize(), archive2.getFilesize());
  EXPECT_EQ(archive1.getClusterCount(), archive2.getClusterCount());

  ASSERT_EQ(archive1.getEntryCount(), archive2.getEntryCount());
  const zim::Entry mainEntry = archive1.getMainEntry();
  ASSERT_EQ(mainEntry.getTitle(), archive2.getMainEntry().getTitle());

  ASSERT_NE(0, archive1.getEntryCount()); // ==> below loop is not a noop
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

  if ( archive1.hasTitleIndex() )
  {
    zim::Search search1(archive1);
    zim::Search search2(archive2);
    search1.set_suggestion_mode(true);
    search2.set_suggestion_mode(true);
    search1.set_query(mainEntry.getTitle());
    search2.set_query(mainEntry.getTitle());
    ASSERT_NE(0, search1.get_matches_estimated());
    ASSERT_EQ(search1.get_matches_estimated(), search2.get_matches_estimated());
    ASSERT_EQ(mainEntry.getPath(), search1.begin().get_url());
    ASSERT_EQ(mainEntry.getPath(), search2.begin().get_url());
    ASSERT_EQ(std::distance(search1.begin(), search1.end()),
              std::distance(search2.begin(), search2.end()));
  }
}

TEST(ZimArchive, multipart)
{
  const zim::Archive archive1("./data/wikibooks_be_all_nopic_2017-02.zim");
  const zim::Archive archive2("./data/wikibooks_be_all_nopic_2017-02_splitted.zim");
  ASSERT_FALSE(archive1.is_multiPart());
  ASSERT_TRUE (archive2.is_multiPart());

  checkEquivalence(archive1, archive2);
}

#ifdef _WIN32
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#undef min
#undef max
# define OPEN_READ_ONLY(path) _open(path, _O_RDONLY)
#else
# define OPEN_READ_ONLY(path) open(path, O_RDONLY)
#endif

TEST(ZimArchive, openByFD)
{
  const zim::Archive archive1("./data/small.zim");
  const int fd = OPEN_READ_ONLY("./data/small.zim");
  const zim::Archive archive2(fd);

  checkEquivalence(archive1, archive2);
}

TEST(ZimArchive, openZIMFileEmbeddedInAnotherFile)
{
  const zim::Archive archive1("./data/small.zim");
  const int fd = OPEN_READ_ONLY("./data/small.zim.embedded");
  const zim::Archive archive2(fd, 8, archive1.getFilesize());

  checkEquivalence(archive1, archive2);
}

} // unnamed namespace
