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
#include <zim/archive.h>
#include <zim/search.h>

#include "gtest/gtest.h"

namespace
{

TEST(Search, searchByTitle)
{
  const zim::Archive archive("./data/small.zim");
  ASSERT_TRUE(archive.hasTitleIndex());
  const zim::Entry mainEntry = archive.getMainEntry();
  zim::Search search(archive);
  search.set_suggestion_mode(true);
  search.set_query(mainEntry.getTitle());
  ASSERT_NE(0, search.get_matches_estimated());
  ASSERT_EQ(mainEntry.getPath(), search.begin().get_url());
}

} // unnamed namespace
