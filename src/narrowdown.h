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

#ifndef ZIM_NARROWDOWN_H
#define ZIM_NARROWDOWN_H

#include "zim_types.h"
#include "debug.h"

#include <algorithm>
#include <vector>

namespace zim
{

// Given a sorted sequence of items with a string key, NarrowDown helps to
// narrow down the range in which the query key should belong.
//
// The target usage of this class is as a partial in-memory index for a sorted
// list residing in external storage with high access cost to inidividual items.
//
// Illustration:
//
// In RAM:
//   key:        A       I       Q       Y       g       o       w  z
//   item #:     |       |       |       |       |       |       |  |
// -----------   |       |       |       |       |       |       |  |
// On disk:      V       V       V       V       V       V       V  V
//   key:        ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
//   data:       ajo097124ljp-oasd)(&(*)llkjasdf@$^nFDSs00ujlasdfjkll
//
// In such an external list looking up an item by key can be performed via a
// binary search where on each iteration the item key must be accessed. There
// are two performance problems with that:
//  1. The API may not allow accessing only the key of the given item, reading
//     the entire item instead (this is the case with dirents).
//  2. Access to items (or only their keys) in external storage is expensive.
//
// NarrowDown speeds up the look-up operation in such an external list by
// allowing to split it into two steps:
//  1. Perform the binary search on the index, yielding a narrower range
//  2. Perform the binary search on the external list starting from that
//     narrower range.
class NarrowDown
{
  typedef article_index_type index_type;

public: // types
  struct Range
  {
    const index_type begin, end;
  };

public: // functions
  void add(const std::string& key, index_type i)
  {
    ASSERT(entries.empty() || entries.back().pseudoKey < key, ==, true);
    ASSERT(entries.empty() || entries.back().lindex < i, ==, true);
    entries.push_back({key, i});
  }

  Range getRange(const std::string& key) const
  {
    auto it = std::upper_bound(entries.begin(), entries.end(), key, LookupPred());
    if ( it == entries.begin() )
      return {0, 0};

    const index_type prevEntryLindex = (it-1)->lindex;

    if ( it == entries.end() )
      return {prevEntryLindex+1, prevEntryLindex+1};

    return {prevEntryLindex, it->lindex};
  }

private: // types

  // pseudoKey is known to belong to the range [lindex, lindex+1)
  // In other words, if were to insert pseudoKey in our sequence of keys
  // it might be placed right after lindex.
  struct Entry
  {
    std::string pseudoKey;
    index_type  lindex;
  };

  struct LookupPred
  {
    bool operator()(const std::string& key, const Entry& entry) const
    {
      return key < entry.pseudoKey;
    }
  };

  typedef std::vector<Entry> EntryCollection;

private: // data
  EntryCollection entries;
};

} // namespace zim

#endif // ZIM_NARROWDOWN_H
