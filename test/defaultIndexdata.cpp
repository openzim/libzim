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


#include <zim/writer/contentProvider.h>

#include "../src/writer/defaultIndexData.h"
#include "gtest/gtest.h"

namespace {

  std::unique_ptr<zim::writer::IndexData> index_data(const std::string& content, const std::string& title)
  {
    std::unique_ptr<zim::writer::ContentProvider> contentProvider(new zim::writer::StringProvider(content));
    return std::unique_ptr<zim::writer::IndexData>(new zim::writer::DefaultIndexData(std::move(contentProvider), title));
  }

  TEST(DefaultIndexdata, empty) {
    auto indexData = index_data("", "A Title");

    ASSERT_EQ(indexData->hasIndexData(), false);
    ASSERT_EQ(indexData->getTitle(), "a title");
    ASSERT_EQ(indexData->getContent(), "");
    ASSERT_EQ(indexData->getKeywords(), "");
    ASSERT_EQ(indexData->getWordCount(), 0U);
    ASSERT_EQ(indexData->getGeoPosition(), std::make_tuple(false, 0, 0));
  }

  TEST(DefaultIndexdata, simple) {
    auto indexData = index_data("<html><body>Some <b>bold</b> words</body><html>", "A Title");

    ASSERT_EQ(indexData->hasIndexData(), true);
    ASSERT_EQ(indexData->getTitle(), "a title");
    ASSERT_EQ(indexData->getContent(), "some bold words");
    ASSERT_EQ(indexData->getKeywords(), "");
    ASSERT_EQ(indexData->getWordCount(), 3U);
    ASSERT_EQ(indexData->getGeoPosition(), std::make_tuple(false, 0, 0));
  }

  TEST(DefaultIndexdata, noindexhead) {
    auto indexData = index_data(R"(<html><head><meta name="robots" content="noindex"></head><body>Some <b>bold</b> words</body><html>)", "A Title");

    ASSERT_EQ(indexData->hasIndexData(), false);
    ASSERT_EQ(indexData->getTitle(), "a title");
    ASSERT_EQ(indexData->getContent(), "");
    ASSERT_EQ(indexData->getKeywords(), "");
    ASSERT_EQ(indexData->getWordCount(), 0U);
    ASSERT_EQ(indexData->getGeoPosition(), std::make_tuple(false, 0, 0));
  }

  TEST(DefaultIndexdata, noindexnone) {
    auto indexData = index_data(R"(<html><head><meta name="robots" content="none"></head><body>Some <b>bold</b> words</body><html>)", "A Title");

    ASSERT_EQ(indexData->hasIndexData(), false);
    ASSERT_EQ(indexData->getTitle(), "a title");
    ASSERT_EQ(indexData->getContent(), "");
    ASSERT_EQ(indexData->getKeywords(), "");
    ASSERT_EQ(indexData->getWordCount(), 0U);
    ASSERT_EQ(indexData->getGeoPosition(), std::make_tuple(false, 0, 0));
  }

 TEST(DefaultIndexdata, noindexbody) {
    auto indexData = index_data("<html><body>NOINDEXSome <b>bold</b> words</body><html>", "A Title");

    ASSERT_EQ(indexData->hasIndexData(), false);
    ASSERT_EQ(indexData->getTitle(), "a title");
    ASSERT_EQ(indexData->getContent(), "noindexsome bold words");
    ASSERT_EQ(indexData->getKeywords(), "");
    ASSERT_EQ(indexData->getWordCount(), 3U);
    ASSERT_EQ(indexData->getGeoPosition(), std::make_tuple(false, 0, 0));
  }

  TEST(DefaultIndexdata, full) {
    auto indexData = index_data(R"(<html><head><meta name="keywords" content="some keyword important"><meta name="geo.position" content="45.005;10.100"></head><body>Some <b>bold</b> words</body><html>)", "A Title");

    ASSERT_EQ(indexData->hasIndexData(), true);
    ASSERT_EQ(indexData->getTitle(), "a title");
    ASSERT_EQ(indexData->getContent(), "some bold words");
    ASSERT_EQ(indexData->getKeywords(), "some keyword important");
    ASSERT_EQ(indexData->getWordCount(), 3U);
    auto geoPos = indexData->getGeoPosition();
    ASSERT_TRUE(std::get<0>(geoPos));
    ASSERT_NEAR(std::get<1>(geoPos), 45.005, 0.00001);
    ASSERT_NEAR(std::get<2>(geoPos), 10.1, 0.00001);
  }
}
