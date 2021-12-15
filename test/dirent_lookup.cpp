/*
 * Copyright (C) 2020 Matthieu Gautier <mgautier@kymeria.fr>
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

#include "../src/dirent_lookup.h"
#include "../src/_dirent.h"
#include <zim/zim.h>

#include "gtest/gtest.h"

#include <vector>
#include <string>
#include <utility>

namespace
{

const std::vector<std::pair<char, std::string>> articleurl = {
  {'A', "aa"},       //0
  {'A', "aaaa"},     //1
  {'A', "aaaaaa"},   //2
  {'A', "aaaabb"},   //3
  {'A', "aaaacc"},   //4
  {'A', "aabbaa"},   //5
  {'A', "aabbbb"},   //6
  {'A', "aabbcc"},   //7
  {'A', "cccccc"},   //8
  {'M', "foo"},      //9
  {'a', "aa"},       //10
  {'a', "bb"},       //11
  {'b', "aa"}        //12
};

struct GetDirentMock
{
  zim::entry_index_t getDirentCount() const {
    return zim::entry_index_t(articleurl.size());
  }

  std::shared_ptr<const zim::Dirent> getDirent(zim::entry_index_t idx) const {
    auto info = articleurl.at(idx.v);
    auto ret = std::make_shared<zim::Dirent>();
    ret->setUrl(info.first, info.second);
    return ret;
  }
};

class NamespaceTest : public :: testing::Test
{
  protected:
    GetDirentMock impl;
};

TEST_F(NamespaceTest, BeginOffset)
{
  ASSERT_EQ(zim::getNamespaceBeginOffset(impl, 'a').v, 10);
  ASSERT_EQ(zim::getNamespaceBeginOffset(impl, 'b').v, 12);
  ASSERT_EQ(zim::getNamespaceBeginOffset(impl, 'c').v, 13);
  ASSERT_EQ(zim::getNamespaceBeginOffset(impl, 'A'-1).v, 0);
  ASSERT_EQ(zim::getNamespaceBeginOffset(impl, 'A').v, 0);
  ASSERT_EQ(zim::getNamespaceBeginOffset(impl, 'M').v, 9);
  ASSERT_EQ(zim::getNamespaceBeginOffset(impl, 'U').v, 10);
}

TEST_F(NamespaceTest, EndOffset)
{
  ASSERT_EQ(zim::getNamespaceEndOffset(impl, 'a').v, 12);
  ASSERT_EQ(zim::getNamespaceEndOffset(impl, 'b').v, 13);
  ASSERT_EQ(zim::getNamespaceEndOffset(impl, 'c').v, 13);
  ASSERT_EQ(zim::getNamespaceEndOffset(impl, 'A'-1).v, 0);
  ASSERT_EQ(zim::getNamespaceEndOffset(impl, 'A').v, 9);
  ASSERT_EQ(zim::getNamespaceEndOffset(impl, 'M').v, 10);
  ASSERT_EQ(zim::getNamespaceEndOffset(impl, 'U').v, 10);
}

TEST_F(NamespaceTest, EndEqualStartPlus1)
{
  for (char ns=32; ns<127; ns++){
    std::cout << "ns: " << ns << "|" << (int)ns << std::endl;
    ASSERT_EQ(zim::getNamespaceEndOffset(impl, ns).v, zim::getNamespaceBeginOffset(impl, ns+1).v);
  }
}


class FindxTest : public :: testing::Test
{
  protected:
    GetDirentMock impl;
};


#define CHECK_FIND_RESULT(expr, is_exact_match, expected_value) \
  { \
    const auto findResult = expr; \
    ASSERT_EQ(findResult.first, is_exact_match); \
    ASSERT_EQ(findResult.second.v, expected_value); \
  }

TEST_F(FindxTest, ExactMatch)
{
#define CHECK_EXACT_MATCH(expr, expected_value)         \
  CHECK_FIND_RESULT(expr, true, expected_value);

  zim::DirentLookup<GetDirentMock> dl(&impl, 4);
  CHECK_EXACT_MATCH(dl.find('A', "aa"), 0);
  CHECK_EXACT_MATCH(dl.find('a', "aa"), 10);
  CHECK_EXACT_MATCH(dl.find('A', "aabbbb"), 6);
  CHECK_EXACT_MATCH(dl.find('b', "aa"), 12);

#undef CHECK_EXACT_MATCH
}


TEST_F(FindxTest, NoExactMatch)
{
#define CHECK_NOEXACT_MATCH(expr, expected_value)        \
  CHECK_FIND_RESULT(expr, false, expected_value);

  zim::DirentLookup<GetDirentMock> dl(&impl, 4);
  CHECK_NOEXACT_MATCH(dl.find('U', "aa"), 10); // No U namespace => return 10 (the index of the first item from the next namespace)
  CHECK_NOEXACT_MATCH(dl.find('A', "aabb"), 5); // aabb is between aaaacc (4) and aabbaa (5) => 5
  CHECK_NOEXACT_MATCH(dl.find('A', "aabbb"), 6); // aabbb is between aabbaa (5) and aabbbb (6) => 6
  CHECK_NOEXACT_MATCH(dl.find('A', "aabbbc"), 7); // aabbbc is between aabbbb (6) and aabbcc (7) => 7
  CHECK_NOEXACT_MATCH(dl.find('A', "bb"), 8); // bb is between aabbcc (7) and cccccc (8) => 8
  CHECK_NOEXACT_MATCH(dl.find('A', "dd"), 9); // dd is after cccccc (8) => 9
  CHECK_NOEXACT_MATCH(dl.find('M', "f"), 9); // f is before foo (9) => 9
  CHECK_NOEXACT_MATCH(dl.find('M', "bar"), 9); // bar is before foo (9) => 9
  CHECK_NOEXACT_MATCH(dl.find('M', "foo1"), 10); // foo1 is after foo (9) => 10

#undef CHECK_NOEXACT_MATCH
}


}  // namespace
