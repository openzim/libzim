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
  auto result = zim::getNamespaceBeginOffset(impl, 'a');
  ASSERT_EQ(result.v, 10);

  result = zim::getNamespaceBeginOffset(impl, 'b');
  ASSERT_EQ(result.v, 12);

  result = zim::getNamespaceBeginOffset(impl, 'c');
  ASSERT_EQ(result.v, 13);

  result = zim::getNamespaceBeginOffset(impl, 'A'-1);
  ASSERT_EQ(result.v, 0);

  result = zim::getNamespaceBeginOffset(impl, 'A');
  ASSERT_EQ(result.v, 0);

  result = zim::getNamespaceBeginOffset(impl, 'M');
  ASSERT_EQ(result.v, 9);

  result = zim::getNamespaceBeginOffset(impl, 'U');
  ASSERT_EQ(result.v, 10);
}

TEST_F(NamespaceTest, EndOffset)
{
  auto result = zim::getNamespaceEndOffset(impl, 'a');
  ASSERT_EQ(result.v, 12);

  result = zim::getNamespaceEndOffset(impl, 'b');
  ASSERT_EQ(result.v, 13);

  result = zim::getNamespaceEndOffset(impl, 'c');
  ASSERT_EQ(result.v, 13);

  result = zim::getNamespaceEndOffset(impl, 'A'-1);
  ASSERT_EQ(result.v, 0);

  result = zim::getNamespaceEndOffset(impl, 'A');
  ASSERT_EQ(result.v, 9);

  result = zim::getNamespaceEndOffset(impl, 'M');
  ASSERT_EQ(result.v, 10);

  result = zim::getNamespaceEndOffset(impl, 'U');
  ASSERT_EQ(result.v, 10);
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

TEST_F(FindxTest, ExactMatch)
{
  zim::DirentLookup<GetDirentMock> dl(&impl, 4);
  auto result = dl.find('A', "aa");
  ASSERT_EQ(result.first, true);
  ASSERT_EQ(result.second.v, 0);

  result = dl.find('a', "aa");
  ASSERT_EQ(result.first, true);
  ASSERT_EQ(result.second.v, 10);

  result = dl.find('A', "aabbbb");
  ASSERT_EQ(result.first, true);
  ASSERT_EQ(result.second.v, 6);

  result = dl.find('b', "aa");
  ASSERT_EQ(result.first, true);
  ASSERT_EQ(result.second.v, 12);
}


TEST_F(FindxTest, NoExactMatch)
{
  zim::DirentLookup<GetDirentMock> dl(&impl, 4);
  auto result = dl.find('U', "aa"); // No U namespace => return 10 (the index of the first item from the next namespace)
  ASSERT_EQ(result.first, false);
  ASSERT_EQ(result.second.v, 10);

  result = dl.find('A', "aabb"); // aabb is between aaaacc (4) and aabbaa (5) => 5
  ASSERT_EQ(result.first, false);
  ASSERT_EQ(result.second.v, 5);

  result = dl.find('A', "aabbb"); // aabbb is between aabbaa (5) and aabbbb (6) => 6
  ASSERT_EQ(result.first, false);
  ASSERT_EQ(result.second.v, 6);

  result = dl.find('A', "aabbbc"); // aabbbc is between aabbbb (6) and aabbcc (7) => 7
  ASSERT_EQ(result.first, false);
  ASSERT_EQ(result.second.v, 7);

  result = dl.find('A', "bb"); // bb is between aabbcc (7) and cccccc (8) => 8
  ASSERT_EQ(result.first, false);
  ASSERT_EQ(result.second.v, 8);

  result = dl.find('A', "dd"); // dd is after cccccc (8) => 9
  ASSERT_EQ(result.first, false);
  ASSERT_EQ(result.second.v, 9);

  result = dl.find('M', "f"); // f is before foo (9) => 9
  ASSERT_EQ(result.first, false);
  ASSERT_EQ(result.second.v, 9);

  result = dl.find('M', "bar"); // bar is before foo (9) => 9
  ASSERT_EQ(result.first, false);
  ASSERT_EQ(result.second.v, 9);

  result = dl.find('M', "foo1"); // foo1 is after foo (9) => 10
  ASSERT_EQ(result.first, false);
  ASSERT_EQ(result.second.v, 10);
}


}  // namespace
