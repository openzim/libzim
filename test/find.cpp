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
// Not found cases

// ByTitle
TEST(FindTests, NotFoundByTitle)
{
    zim::File file ("./test/wikibooks_be_all_nopic_2017-02.zim");

    auto article1 = file.findByTitle('U', "unkownTitle");
    auto article2 = file.findByTitle('A', "unkownTitle");
    ASSERT_EQ(article1->getIndex(), 0);
    ASSERT_EQ(article2->getIndex(), 7);
}

// By URL
TEST(FindTests, NotFoundByURL)
{
    zim::File file ("./test/wikibooks_be_all_nopic_2017-02.zim");

    auto article1 = file.find('U', "unkwonUrl");
    auto article2 = file.find('A', "unkwonUrl");
    ASSERT_EQ(article1->getIndex(), 0);
    ASSERT_EQ(article2->getIndex(), 7);
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
    ASSERT_EQ(article2->getIndex(), 7);
}

// Found cases

// ByTitle
TEST(FindTests, ByTitle)
{
    zim::File file ("./test/wikibooks_be_all_nopic_2017-02.zim");

    auto article1 = file.findByTitle('-', "j/body.js");
    auto article2 = file.findByTitle('A', "index.html");
    ASSERT_EQ(article1.getIndex(), 1);
    ASSERT_EQ(article2->getIndex(), 7);
}

// By URL
TEST(FindTests, ByURL)
{
    zim::File file ("./test/wikibooks_be_all_nopic_2017-02.zim");

    auto article1 = file.find('-', "j/body.js");
    auto article2 = file.find('I', "m/115a35549794e50dcd03e60ef1a1ae24.png");
    ASSERT_EQ(article1->getIndex(), 1);
    ASSERT_EQ(article2->getIndex(), 76);
}

// By URL (no ns)
TEST(FindTests, ByURLDefaultNS)
{
    zim::File file ("./test/wikibooks_be_all_nopic_2017-02.zim");

    auto article0 = file.find("A/Main_Page.html");
    auto article1 = file.find("I/s/ajax-loader.gif");
    auto article2 = file.find("-/j/head.js");
    ASSERT_EQ(article0->getIndex(), 5);
    ASSERT_EQ(article1->getIndex(), 80);
    ASSERT_EQ(article2->getIndex(), 2);
}

} // namespace
