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
#include <zim/archive.h>
#include <zim/error.h>

#include "gtest/gtest.h"

namespace
{
// Not found cases

// ByTitle
TEST(FindTests, NotFoundByTitle)
{
    zim::Archive archive ("./data/wikibooks_be_all_nopic_2017-02.zim");

    auto it = archive.findByTitle("unkownTitle");
    ASSERT_EQ(it, archive.end<zim::EntryOrder::titleOrder>());
}

// By Path
TEST(FindTests, NotFoundByPath)
{
    zim::Archive archive ("./data/wikibooks_be_all_nopic_2017-02.zim");

    auto it0 = archive.findByPath("unkwonUrl");
    auto it1 = archive.findByPath("U/unkwonUrl");
    auto it2 = archive.findByPath("A/unkwonUrl");
    ASSERT_EQ(it0, archive.end<zim::EntryOrder::pathOrder>());
    ASSERT_EQ(it1, archive.end<zim::EntryOrder::pathOrder>());
    ASSERT_EQ(it2, archive.end<zim::EntryOrder::pathOrder>());
}

// Found cases

// ByTitle
TEST(FindTests, ByTitle)
{
    zim::Archive archive ("./data/wikibooks_be_all_nopic_2017-02.zim");

    auto it1 = archive.findByTitle("j/body.js");
    auto it2 = archive.findByTitle("Main Page");
    ASSERT_EQ(it1->getIndex(), 1);
    ASSERT_EQ(it2->getIndex(), 5);
}

// By Path (compatibility)
TEST(FindTests, ByPathNoNS)
{
    zim::Archive archive ("./data/wikibooks_be_all_nopic_2017-02.zim");

    auto it1 = archive.findByPath("j/body.js");
    auto it2 = archive.findByPath("m/115a35549794e50dcd03e60ef1a1ae24.png");
    ASSERT_EQ(it1->getIndex(), 1);
    ASSERT_EQ(it2->getIndex(), 76);
}

// By Path
TEST(FindTests, ByPath)
{
    zim::Archive archive ("./data/wikibooks_be_all_nopic_2017-02.zim");

    auto it0 = archive.findByPath("A/Main_Page.html");
    auto it1 = archive.findByPath("I/s/ajax-loader.gif");
    auto it2 = archive.findByPath("-/j/head.js");
    ASSERT_EQ(it0->getIndex(), 5);
    ASSERT_EQ(it1->getIndex(), 80);
    ASSERT_EQ(it2->getIndex(), 2);
}

} // namespace
