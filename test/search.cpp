/*
 * Copyright (C) 2020 Veloman Yunkan
 * Copyright (C) 2021 Maneesh P M <manu.pm55@gmail.com>
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
#include <zim/archive.h>
#include <zim/search.h>
#include <zim/writer/creator.h>
#include <zim/writer/item.h>
#include <zim/writer/contentProvider.h>

#include "tools.h"

#include "tools.h"
#include "gtest/gtest.h"

namespace
{

using zim::unittests::getDataFilePath;

class TestItem : public zim::writer::Item
{
  public:
    TestItem(const std::string& path, const std::string& title, const std::string& content):
     path(path), title(title), content(content)  { }
    virtual ~TestItem() = default;

    virtual std::string getPath() const { return path; };
    virtual std::string getTitle() const { return title; };
    virtual std::string getMimeType() const { return "text/html"; };

    virtual std::unique_ptr<zim::writer::ContentProvider> getContentProvider() const {
      return std::unique_ptr<zim::writer::ContentProvider>(new zim::writer::StringProvider(content));
    }

  std::string path;
  std::string title;
  std::string content;
};

// Helper class to create a temporary zim and remove it once the test is done
class TempZimArchive : zim::unittests::TempFile
{
  public:
    explicit TempZimArchive(const char* tempPath) : zim::unittests::TempFile {tempPath} {}

    const std::string getPath() {
      return this->path();
    }
};

#if WITH_TEST_DATA
TEST(Search, searchByTitle)
{
  for(auto& path:getDataFilePath("small.zim")) {
    const zim::Archive archive(path);
    ASSERT_TRUE(archive.hasTitleIndex());
    const zim::Entry mainEntry = archive.getMainEntry();
    zim::Search search(archive);
    search.set_suggestion_mode(true);
    search.set_query(mainEntry.getTitle());
    search.set_range(0, archive.getEntryCount());
    ASSERT_NE(0, search.get_matches_estimated());
    ASSERT_EQ(mainEntry.getPath(), search.begin().get_path());
  }
}
#endif

// To secure compatibity of new zim files with older kiwixes, we need to index
// full path of the entries as data of documents.
TEST(Search, indexFullPath)
{
  TempZimArchive tza("testZim");
  zim::writer::Creator creator;
  creator.configIndexing(true, "en");
  creator.startZimCreation(tza.getPath());

  auto item = std::make_shared<TestItem>("testPath", "Test Article", "This is a test article");
  creator.addItem(item);

  creator.setMainPath("testPath");
  creator.addMetadata("Title", "Test zim");
  creator.finishZimCreation();

  zim::Archive archive(tza.getPath());

  zim::Search search(archive);
  search.set_suggestion_mode(false);
  search.set_query("test article");
  search.set_range(0, archive.getEntryCount());
  search.set_verbose(true);

  ASSERT_EQ(search.begin().get_path(), "testPath");
  ASSERT_EQ(search.begin().get_dbData().substr(0, 2), "C/");
}

} // unnamed namespace
