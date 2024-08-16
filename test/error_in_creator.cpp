/*
 * Copyright (C) 2022 Matthieu Gautier <mgautier@kymeria.fr>
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
#include "../src/tools.h"
#include <zim/error.h>
#include "gtest/gtest.h"
#include "config.h"

namespace
{

using namespace zim;

enum class ERRORKIND {
  PATH,                                  // 0
  TITLE,                                 // 1
  MIMETYPE,                              // 2
  HINTS,                                 // 3
  GET_CONTENTPROVIDER,                   // 4
  EXCEPTION_CONTENTPROVIDER_SIZE,        // 5
  EXCEPTION_CONTENTPROVIDER_FEED,        // 6
  WRONG_OVER_SIZE_CONTENTPROVIDER,       // 7
  WRONG_UNDER_SIZE_CONTENTPROVIDER,      // 8
  GET_INDEXDATA,                         // 9
  HAS_INDEXDATA,                         // 10
  GET_INDEXDATA_TITLE,                   // 11
  GET_INDEXDATA_CONTENT,                 // 12
  GET_INDEXDATA_KEYWORD,                 // 13
  GET_INDEXDATA_WORDCOUNT,               // 14
  GET_INDEXDATA_POSITION                 // 15
};

class SimulatedFaultError : public std::runtime_error
{
  public:
    SimulatedFaultError(const std::string& err) : std::runtime_error(err) {}
};

#define THROW_ERROR(KIND) if (ERRORKIND::KIND == fault) { throw SimulatedFaultError(#KIND); } else {}

class FaultyContentProvider : public writer::StringProvider
{
  public:
    FaultyContentProvider(const std::string& content, ERRORKIND fault)
      : writer::StringProvider(content),
        fault(fault)
    {}

  zim::size_type getSize() const override {
    THROW_ERROR(EXCEPTION_CONTENTPROVIDER_SIZE)
    auto size = writer::StringProvider::getSize();
    if (fault == ERRORKIND::WRONG_OVER_SIZE_CONTENTPROVIDER) {
      return size + 1;
    }
    if (fault == ERRORKIND::WRONG_UNDER_SIZE_CONTENTPROVIDER) {
      return size - 1;
    }
    return size;
  }

  Blob feed() override {
    THROW_ERROR(EXCEPTION_CONTENTPROVIDER_FEED)
    return writer::StringProvider::feed();
  }

  private:
    ERRORKIND fault;
};


class FaultyIndexData : public writer::IndexData
{
  public:
    explicit FaultyIndexData(ERRORKIND fault)
     : fault(fault)
    {}

    bool hasIndexData() const override {
      THROW_ERROR(HAS_INDEXDATA)
      return true;
    }

    std::string getTitle() const override {
      THROW_ERROR(GET_INDEXDATA_TITLE)
      return "Foo";
    }

    std::string getContent() const override {
      THROW_ERROR(GET_INDEXDATA_CONTENT)
      return "FooContent";
    }

    std::string getKeywords() const override {
      THROW_ERROR(GET_INDEXDATA_KEYWORD)
      return "";
    }

    uint32_t getWordCount() const override {
      THROW_ERROR(GET_INDEXDATA_WORDCOUNT)
      return 1;
    }

    GeoPosition getGeoPosition() const override {
      THROW_ERROR(GET_INDEXDATA_POSITION)
      return GeoPosition();
    }

  private:
    ERRORKIND fault;
};


class FaultyItem : public writer::Item
{
  public:
    FaultyItem(const std::string& path, const std::string& title, const std::string& content, bool compressed, ERRORKIND fault):
     path(path), title(title), content(content), compressed(compressed), fault(fault)  { }
    virtual ~FaultyItem() = default;

    std::string getPath() const override { THROW_ERROR(PATH) return path; };
    std::string getTitle() const override { THROW_ERROR(TITLE) return title; };
    std::string getMimeType() const override { THROW_ERROR(MIMETYPE) return "text/html"; };
    writer::Hints getHints() const override {
      THROW_ERROR(HINTS)
      return { { writer::COMPRESS, compressed } };
    }

    std::unique_ptr<writer::ContentProvider> getContentProvider() const override {
      THROW_ERROR(GET_CONTENTPROVIDER)
      return std::unique_ptr<writer::ContentProvider>(new FaultyContentProvider(content, fault));
    }

    std::shared_ptr<writer::IndexData> getIndexData() const override {
      THROW_ERROR(GET_INDEXDATA)
      return std::shared_ptr<writer::IndexData>(new FaultyIndexData(fault));
    }

  std::string path;
  std::string title;
  std::string content;
  bool compressed;
  ERRORKIND fault;
};


class FaultyItemErrorTest : public ::testing::TestWithParam<ERRORKIND> {};

TEST_P(FaultyItemErrorTest, faultyItem)
{
  unittests::TempFile temp("zimfile");
  auto tempPath = temp.path();

  writer::Creator creator;
  creator.configIndexing(true, "eng");
  creator.startZimCreation(tempPath);
  auto item = std::make_shared<FaultyItem>("foo", "Foo", "FooContent", true, GetParam());
  // A error in the item is directly reported
  EXPECT_THROW(creator.addItem(item), SimulatedFaultError);
  // As the error is directly reported, finishZimCreation report nothing.
  EXPECT_NO_THROW(creator.finishZimCreation());
}

const auto errorKinds = {
  ERRORKIND::PATH,
  ERRORKIND::TITLE,
  ERRORKIND::MIMETYPE,
  ERRORKIND::HINTS,
  ERRORKIND::GET_CONTENTPROVIDER,
  ERRORKIND::EXCEPTION_CONTENTPROVIDER_SIZE,
#if defined(ENABLE_XAPIAN)
  ERRORKIND::GET_INDEXDATA,
#endif //ENABLE_XAPIAN
};
INSTANTIATE_TEST_SUITE_P(
CreatorError,
FaultyItemErrorTest,
::testing::ValuesIn(errorKinds));

double getWaitTimeFactor() {
  char* str_time_factor = std::getenv("WAIT_TIME_FACTOR_TEST");
  if (str_time_factor) {
    // Yes, if user set a "non float" value, sleep_time will be 0. But not our problem here
    // Same thing if user set a negative value.
    return atof(str_time_factor);
  }
  return 1.0;
}


class FaultyDelayedItemErrorTest : public ::testing::TestWithParam<ERRORKIND> {};


// All the following code "should" thrown a error :
// - An asyncError on the first call (async_error_thrown == false)
// - An CreatorStateError on other calls (async_error_thrown == true)
// But it may not thrown if worker thread has not run.
// (and only in this case. If a AsyncError has been thrown, other calls MUST throw a CreatorErrorState).
// FinishZimCreation() always waits for workers, so we MUST have an exception.
#define CHECK_ASYNC_EXCEPT(CALL) \
  { \
    const char* something_went_wrong = nullptr; \
    const bool MUST_FAIL = std::string(#CALL).find("finishZimCreation") != std::string::npos; \
    try { \
      CALL; \
      if (async_error_thrown) { \
        something_went_wrong = "We should have thrown a CreatorStateError after AsyncError has been detected."; \
      } \
      if (MUST_FAIL) { \
        something_went_wrong = "The call should have thrown an exception."; \
      } \
    } catch (AsyncError& e) { \
      if (async_error_thrown) { \
        something_went_wrong = "We should have thrown a CreatorStateError after AsyncError has been detected."; \
      } \
      async_error_thrown = true; \
    } catch (CreatorStateError& e) { \
      if (!async_error_thrown) { \
        something_went_wrong = "CreatorStateError must be thrown after a AsyncError."; \
      } \
    } catch (...) { \
      something_went_wrong = "An exception other than CreatorStateError or AsyncError was thrown."; \
    } \
    if (something_went_wrong) { \
      FAIL() << something_went_wrong; \
    } \
  }


// Compressed and uncompressed content use a different code path as
// compressed cluster uses contentProvider when the cluster is closed (compressed)
// and uncompressed cluster uses contentProvider when the cluster is written.
TEST_P(FaultyDelayedItemErrorTest, faultyCompressedItem)
{
  unittests::TempFile temp("zimfile");
  auto tempPath = temp.path();

  bool async_error_thrown = false;
  writer::Creator creator;
  creator.configIndexing(true, "eng");
  creator.configClusterSize(5);
  creator.startZimCreation(tempPath);
  auto item = std::make_shared<FaultyItem>("foo", "Foo", "FooContent", true, GetParam());
  // Exception is not thrown in main thread so error is not detected
  EXPECT_NO_THROW(creator.addItem(item));
  // We force the closing of the cluster, so working thread will detect error
  CHECK_ASYNC_EXCEPT(creator.addMetadata("A metadata", "A compressed (default) metadata"));
  // give a chance to threads to detect the error.
  // How many time to wait is a bit tricky.
  // Too long and all tests will wait to much and developpers hate to wait,
  // Not enough and error is not deteced and tests fail (and developpers hate failing tets)
  // The exact value is specific to each computer, so we need to make this configurable.
  // We use a base and we multiply it by a factor taken from env variable.
  const long sleep_time = 1000000; // Default value is set to a factor 10 above what is needed to work on my (fast) computer
  zim::microsleep((int)(sleep_time * getWaitTimeFactor()));
  // We detect it for any call after
  CHECK_ASYNC_EXCEPT(creator.addMetadata("Title", "This is a title"));
  CHECK_ASYNC_EXCEPT(creator.addIllustration(48, "PNGBinaryContent48"));
  CHECK_ASYNC_EXCEPT(creator.addRedirection("foo2", "FooRedirection", "foo"));
  CHECK_ASYNC_EXCEPT(creator.finishZimCreation());
}

TEST_P(FaultyDelayedItemErrorTest, faultyUnCompressedItem)
{
  unittests::TempFile temp("zimfile");
  auto tempPath = temp.path();

  bool async_error_thrown = false;
  writer::Creator creator;
  creator.configIndexing(true, "eng");
  creator.configClusterSize(5);
  creator.startZimCreation(tempPath);
  auto item = std::make_shared<FaultyItem>("foo", "Foo", "FooContent", false, GetParam());
  // Exception is not thrown in main thread so error is not detected
  EXPECT_NO_THROW(creator.addItem(item));
  // We force the closing of the cluster, so working thread will detect error
  CHECK_ASYNC_EXCEPT(creator.addMetadata("A metadata", "A uncompressed metadata", "plain/content"));
  // give a chance to threads to detect the error
  // How many time to wait is a bit tricky.
  // Too long and all tests will wait to much and developpers hate to wait
  // Not enough and error is not deteced and tests fail (and developpers hate failing tets)
  // The exacte value is specific to each computer, so we need to make this configurable.
  // We use a base and we multiply it by a factor taken from env variable.
  // Note here, that we have a base smaller than for compressed test as we don't compress the content
  // and the writer thread (the one using the contentProvider) detect the error sooner
  const long sleep_time = 10000; // Default value is set to a factor 10 above what is needed to work on my (fast) computer
  zim::microsleep((int)(sleep_time * getWaitTimeFactor()));
  // But we detect it for any call after
  CHECK_ASYNC_EXCEPT(creator.addMetadata("Title", "This is a title"));
  CHECK_ASYNC_EXCEPT(creator.addIllustration(48, "PNGBinaryContent48"));
  CHECK_ASYNC_EXCEPT(creator.addRedirection("foo2", "FooRedirection", "foo"));
  CHECK_ASYNC_EXCEPT(creator.finishZimCreation());
}


// Check that destructor correctly clean everything on error
// even if finishZimCreation is not called.
TEST_P(FaultyDelayedItemErrorTest, faultyUnfinishedCreator)
{
  unittests::TempFile tmpFile("zimfile");
  {
    writer::Creator creator;
    creator.configIndexing(true, "eng");
    creator.configClusterSize(5);
    creator.startZimCreation(tmpFile.path());
    auto item = std::make_shared<FaultyItem>("foo", "Foo", "FooContent", true, GetParam());
    // Exception is not thrown in main thread so error is not detected
    EXPECT_NO_THROW(creator.addItem(item));
    // creator.finishZimCreation() is not called
  }

  EXPECT_THROW(
      {
        const zim::Archive archive(tmpFile.path());
      },
      zim::ZimFileFormatError
  );
}
const auto delayedErrorKinds = {
  ERRORKIND::EXCEPTION_CONTENTPROVIDER_FEED,
  ERRORKIND::WRONG_OVER_SIZE_CONTENTPROVIDER,
  ERRORKIND::WRONG_UNDER_SIZE_CONTENTPROVIDER,
#if defined(ENABLE_XAPIAN)
  ERRORKIND::HAS_INDEXDATA,
  ERRORKIND::GET_INDEXDATA_TITLE,
  ERRORKIND::GET_INDEXDATA_CONTENT,
  ERRORKIND::GET_INDEXDATA_KEYWORD,
  ERRORKIND::GET_INDEXDATA_WORDCOUNT,
  ERRORKIND::GET_INDEXDATA_POSITION,
#endif // ENABLE_XAPIAN
};
INSTANTIATE_TEST_SUITE_P(
CreatorError,
FaultyDelayedItemErrorTest,
::testing::ValuesIn(delayedErrorKinds));
} // unnamed namespace
