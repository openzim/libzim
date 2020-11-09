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

#include <zim/zim.h>
#include <zim/file.h>

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
emptyZimFileContent()
{
  std::string content;
  content += "ZIM\x04"; // Magic
  content += "\x05" + std::string(3, '\0'); // Version
  content += std::string(16, '\0'); // uuid
  content += std::string(4, '\0'); // article count
  content += std::string(4, '\0'); // cluster count
  content += "\x50" + std::string(7, '\0'); // url ptr pos
  content += "\x50" + std::string(7, '\0'); // title ptr pos
  content += "\x50" + std::string(7, '\0'); // cluster ptr pos
  content += "\x50" + std::string(7, '\0'); // mimelist ptr pos
  content += std::string(4, '\0'); // main page index
  content += std::string(4, '\0'); // layout page index
  content += "\x50" + std::string(7, '\0'); // checksum pos
  content += "\x8a\xbb\xad\x98\x64\xd5\x48\xb2\xb9\x71\xab\x30\xed\x29\xa4\x01"; // md5sum
  return content;
}

std::unique_ptr<TempFile>
makeTempFile(const char* name, const std::string& content)
{
  std::unique_ptr<TempFile> p(new TempFile(name));
  write(p->fd(), &content[0], content.size());
  return p;
}


TEST(ZimFile, openingAnInvalidZimFileFails)
{
  const char* const prefixes[] = { "ZIM\x04", "" };
  const unsigned char bytes[] = {0x00, 0x01, 0x11, 0x30, 0xFF};
  for ( const std::string prefix : prefixes ) {
    for ( const unsigned char byte : bytes ) {
      for ( int count = 0; count < 100; count += 10 ) {
        const TestContext ctx{
                {"prefix",  prefix.size() ? "yes" : "no" },
                {"byte", std::to_string(byte) },
                {"count", std::to_string(count) }
        };
        const std::string zimfileContent = prefix + std::string(count, byte);
        const auto tmpfile = makeTempFile("invalid_zim_file", zimfileContent);

        EXPECT_THROW( zim::File(tmpfile->path()), std::runtime_error ) << ctx;
      }
    }
  }
}

TEST(ZimFile, openingAnEmptyZimFileSucceeds)
{
  const auto tmpfile = makeTempFile("empty_zim_file", emptyZimFileContent());

  zim::File zimfile(tmpfile->path());
  ASSERT_TRUE(zimfile.verify());
}

bool isNastyOffset(int offset) {
  if ( 6 <= offset && offset < 24 ) // Minor version or uuid
    return false;

  if ( 64 <= offset && offset < 72 ) // page or layout index
    return false;

  return true;
}

TEST(ZimFile, nastyEmptyZimFile)
{
  const std::string correctContent = emptyZimFileContent();
  for ( int offset = 0; offset < 80; ++offset ) {
    if ( isNastyOffset(offset) ) {
      const TestContext ctx{ {"offset", std::to_string(offset) } };
      std::string nastyContent(correctContent);
      nastyContent[offset] = '\xff';
      const auto tmpfile = makeTempFile("wrong_checksum_empty_zim_file", nastyContent);
      EXPECT_THROW( zim::File(tmpfile->path()), std::runtime_error ) << ctx;
    }
  }
}

TEST(ZimFile, wrongChecksumInEmptyZimFile)
{
  std::string zimfileContent = emptyZimFileContent();
  zimfileContent[85] = '\xff';
  const auto tmpfile = makeTempFile("wrong_checksum_empty_zim_file", zimfileContent);

  zim::File zimfile(tmpfile->path());
  ASSERT_FALSE(zimfile.verify());
}

TEST(ZimFile, openRealZimFile)
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
    std::unique_ptr<zim::File> zimfile;
    EXPECT_NO_THROW( zimfile.reset(new zim::File(path)) ) << ctx;
    if ( zimfile ) {
      EXPECT_TRUE( zimfile->verify() ) << ctx;
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

#define EXPECT_STDERR(x) EXPECT_EQ(x, std::string(stderror))
#define EXPECT_BROKEN_ZIMFILE(zimpath, expected_stderror_text)    \
  {                                                               \
    CapturedStderr stderror;                                      \
    EXPECT_FALSE(zim::validate(zimpath, all));                    \
    EXPECT_EQ(expected_stderror_text, std::string(stderror));     \
  }

TEST(ZimFile, validate)
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
}

TEST(ZimFile, multipart)
{
  const zim::File zimfile1("./data/wikibooks_be_all_nopic_2017-02.zim");
  const zim::File zimfile2("./data/wikibooks_be_all_nopic_2017-02_splitted.zim");
  ASSERT_FALSE(zimfile1.is_multiPart());
  ASSERT_TRUE (zimfile2.is_multiPart());

  EXPECT_EQ(zimfile1.getFilesize(), zimfile2.getFilesize());
  EXPECT_EQ(zimfile1.getCountClusters(), zimfile2.getCountClusters());
  EXPECT_EQ(zimfile1.getNamespaces(), zimfile2.getNamespaces());

  ASSERT_EQ(zimfile1.getCountArticles(), zimfile2.getCountArticles());

  ASSERT_EQ(118, zimfile1.getCountArticles()); // ==> below loop is not a noop
  for ( zim::article_index_type i = 0; i < zimfile1.getCountArticles(); ++i ) {
    zim::Article article1 = zimfile1.getArticle(i);
    zim::Article article2 = zimfile2.getArticle(i);
    ASSERT_EQ(i, article1.getIndex());
    ASSERT_EQ(i, article2.getIndex());
    ASSERT_EQ(article1.getClusterNumber(), article2.getClusterNumber());
    ASSERT_EQ(article1.getOffset(), article2.getOffset());
    ASSERT_EQ(article1.getParameter(), article2.getParameter());
    ASSERT_EQ(article1.getTitle(), article2.getTitle());
    ASSERT_EQ(article1.getUrl(), article2.getUrl());
    ASSERT_EQ(article1.getLongUrl(), article2.getLongUrl());
    ASSERT_EQ(article1.getLibraryMimeType(), article2.getLibraryMimeType());
    ASSERT_EQ(article1.isRedirect(), article2.isRedirect());
    ASSERT_EQ(article1.isLinktarget(), article2.isLinktarget());
    ASSERT_EQ(article1.isDeleted(), article2.isDeleted());
    ASSERT_EQ(article1.getNamespace(), article2.getNamespace());
    ASSERT_EQ(article1.getArticleSize(), article2.getArticleSize());
    ASSERT_EQ(article1.getData(), article2.getData());
    if ( !article1.isRedirect() && ! article1.isLinktarget() && !article1.isLinktarget() ) {
      ASSERT_EQ(article1.getPage(true, 5), article2.getPage(true, 5));
      ASSERT_EQ(article1.getPage(false, 5), article2.getPage(false, 5));
    }
    ASSERT_EQ(zimfile1.getArticleByTitle(i).getIndex(),
              zimfile2.getArticleByTitle(i).getIndex()
    );
    ASSERT_EQ(zimfile1.getArticleByClusterOrder(i).getIndex(),
              zimfile2.getArticleByClusterOrder(i).getIndex()
    );
  }
}

} // unnamed namespace
