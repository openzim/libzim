/*
 * Copyright (C) 2009 Miguel Rocha
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
#include <zim/error.h>
#include <zim/fileiterator.h>

#include "gtest/gtest.h"

namespace
{


TEST(ClusterIteratorTest, getArticleByClusterOrder)
{
    std::vector<zim::article_index_type> expected = {
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 109, 110, 111, 112, 113, 114, 115, 116,
117, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94,
95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108 };

    zim::File file ("./test/wikibooks_be_all_nopic_2017-02.zim");

    auto nbArticles = file.getCountArticles();

    ASSERT_EQ(nbArticles, expected.size());

    for (auto i = 0u; i < nbArticles; i++)
    {
        EXPECT_EQ(file.getArticleByClusterOrder(i).getIndex(), expected[i]);
    }
}

TEST(getArticle, indexOutOfRange)
{
    zim::File file ("./test/wikibooks_be_all_nopic_2017-02.zim");

    auto nbArticles = file.getCountArticles();

    try {
        file.getArticle(nbArticles);
        FAIL() << "Should throw exception\n";
    }  catch (zim::ZimFileFormatError &e) {
        ASSERT_EQ(e.what(), std::string("article index out of range"));
    }  catch(...) {
        FAIL() << "Should throw exception\n";
    }
}

// ByTitle
TEST(IteratorTests, begin)
{
    std::vector<zim::article_index_type> expected = {
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 109, 110, 111, 112, 113, 114, 115, 116,
117, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94,
95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108 };

    zim::File file ("./test/wikibooks_be_all_nopic_2017-02.zim");

    auto it = file.begin();
    int i = 0;
    while (it != file.end())
    {
        EXPECT_EQ(it->getIndex(), expected[i]);
        it++; i++;
    }
}


// ByTitle
TEST(IteratorTests, beginByTitle)
{
    std::vector<zim::article_index_type> expected = { 0, 1, 2, 3, 4, 5, 7, 8, 9, 10};
    zim::File file ("./test/wikibooks_be_all_nopic_2017-02.zim");

    auto it = file.beginByTitle();

    int i = 0;
    while (i < 10)
    {
        EXPECT_EQ(it->getIndex(), expected[i]);
        it++; i++;
    }
    std::cout << "\n";
}


// ByUrl
TEST(IteratorTests, beginByUrl)
{
    std::vector<zim::article_index_type> expected = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    zim::File file ("./test/wikibooks_be_all_nopic_2017-02.zim");

    auto it = file.beginByUrl();
    int i = 0;
    while (i < 10)
    {
        EXPECT_EQ(it->getIndex(), expected[i]);
        it++; i++;
    }
    std::cout << "\n";
}

// Not found cases

// ByTitle
TEST(FindTests, NotFoundByTitle)
{

    zim::File file ("./test/wikibooks_be_all_nopic_2017-02.zim");

    auto article1 = file.findByTitle('U', "unkownTitle");
    auto article2 = file.findByTitle('A', "unkownTitle");
    ASSERT_EQ(article1->getIndex(), 0);

    // TODO: findx returns a >0 index even when it doesnt find a article
    // ASSERT_EQ(article2->getIndex(), 0);
}

// By URL
TEST(FindTests, NotFoundByURL)
{
    zim::File file ("./test/wikibooks_be_all_nopic_2017-02.zim");

    auto article1 = file.find('U', "unkwonUrl");
    auto article2 = file.find('A', "unkwonUrl");
    ASSERT_EQ(article1->getIndex(), 0);

    // TODO: findx returns a >0 index even when it doesnt find a article
    // ASSERT_EQ(article2->getIndex(), 0);
}

// By URL (no ns)
TEST(FindTests, NotFoundByURLDefaultNS)
{
    zim::File file ("./test/wikibooks_be_all_nopic_2017-02.zim");

    auto article0 = file.find("unkwonUrl");
    auto article1 = file.find("U/unkwonUrl");
    auto article2 = file.find("A/unkwonUrl");
    ASSERT_EQ(article0->getIndex(), 0);
    ASSERT_EQ(article1->getIndex(), 0);


    // TODO: findx returns a >0 index even when it doesnt find a article
    // ASSERT_EQ(article2->getIndex(), 0);
}

// Found cases

// ByTitle
TEST(FindTests, ByTitle)
{
    zim::File file ("./test/wikibooks_be_all_nopic_2017-02.zim");

    auto article1 = file.findByTitle('U', "unkownTitle");
    auto article2 = file.findByTitle('A', "unkownTitle");
    ASSERT_EQ(article1->getIndex(), 0);

    // TODO: findx returns a >0 index even when it doesnt find a article
    // ASSERT_EQ(article2->getIndex(), 0);
}

// By URL
TEST(FindTests, ByURL)
{
    zim::File file ("./test/wikibooks_be_all_nopic_2017-02.zim");

    auto article1 = file.find('U', "unkwonUrl");
    auto article2 = file.find('A', "unkwonUrl");
    ASSERT_EQ(article1->getIndex(), 0);

    // TODO: findx returns a >0 index even when it doesnt find a article
    // ASSERT_EQ(article2->getIndex(), 0);
}

// By URL (no ns)
TEST(FindTests, ByURLDefaultNS)
{
    zim::File file ("./test/wikibooks_be_all_nopic_2017-02.zim");

    auto article0 = file.find("unkwonUrl");
    auto article1 = file.find("U/unkwonUrl");
    auto article2 = file.find("A/unkwonUrl");
    ASSERT_EQ(article0->getIndex(), 0);
    ASSERT_EQ(article1->getIndex(), 0);

    // TODO: findx returns a >0 index even when it doesnt find a article
    // ASSERT_EQ(article2->getIndex(), 0);
}

} // namespace
