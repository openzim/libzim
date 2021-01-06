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

#include <zim/error.h>

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
//
// The denser the in-memory index the more the performance improvement.
// Therefore the implementation focus of NarrowDown is on small memory
// footprint. If the item keys are long strings with a lot of "garbage" at the
// end the following trick helps. Suppose that we have the following pair of
// adjacent keys in our full (external) list:
//
// Item # | Key
// ---------------------------------
// ...    | ...
// 1234   | "We Are The Champions"
// 1235   | "We Will Rock You"
// ...    | ...
//
// If we were to include the item #1234 in our index the naive approach would
// be to store its key as is. However, let's imagine that the list also
// contains an item with key "We W". Then it would have to reside between "We
// Are The Champions" and "We Will Rock You". So we can pretend that such an
// item exists and store in our index the fictitious entry {"We W", 1234.5}.
// When we arrive at that entry during the range narrow-down step we must round
// the item index downward if it is going to be used as the lower bound of
// the range, and round it upward if it is going to be used as the upper bound
// of the range.
class NarrowDown
{
  typedef entry_index_type index_type;

public: // types
  struct Range
  {
    const index_type begin, end;
  };

public: // functions
  NarrowDown()
    : pred(&keyContentArea)
  {}

  // Add another entry to the search index. The key of the next item is used
  // to derive and store a shorter pseudo-key as explained in the long comment
  // above the class.
  void add(const std::string& key, index_type i, const std::string& nextKey)
  {
    if (key >= nextKey) {
      std::stringstream ss;
      ss << "Dirent table is not properly sorted:\n";
      ss << "  #0: " << key[0] << "/" << key.substr(1) << "\n";
      ss << "  #1: " << nextKey[0] << "/" << nextKey.substr(1);
      throw ZimFileFormatError(ss.str());
    }
    if ( entries.empty() ) {
      addEntry(key, i);
    } else {
      const std::string pseudoKey = shortestStringInBetween(key, nextKey);
      if (!pred(entries.back(), pseudoKey)) {
        std::stringstream ss;
        ss << "Dirent table is not properly sorted:\n";
        ss << "  #0: " << key[0] << "/" << key.substr(1) << "\n";
        ss << "  #1: " << nextKey[0] << "/" << nextKey.substr(1);
        throw ZimFileFormatError(ss.str());
      }
      ASSERT(entries.back().lindex, <, i);
      addEntry(pseudoKey, i);
    }
  }

  void close(const std::string& key, index_type i)
  {
    ASSERT(entries.empty() || pred(entries.back(), key), ==, true);
    ASSERT(entries.empty() || entries.back().lindex < i, ==, true);
    addEntry(key, i);
  }

  Range getRange(const std::string& key) const
  {
    auto it = std::upper_bound(entries.begin(), entries.end(), key, pred);
    if ( it == entries.begin() )
      return {0, 0};

    const index_type prevEntryLindex = (it-1)->lindex;

    if ( it == entries.end() )
      return {prevEntryLindex, prevEntryLindex+1};

    return {prevEntryLindex, it->lindex+1};
  }

  static std::string shortestStringInBetween(const std::string& a, const std::string& b)
  {
    ASSERT(a, <=, b);

    // msvc version of `std::mismatch(begin1, end1, begin2)`
    // need `begin2 + (end1-begin1)` to be valid.
    // So we cannot simply pass `a.end()` as `end1`.
    const auto minlen = std::min(a.size(), b.size());
    const auto m = std::mismatch(a.begin(), a.begin()+minlen, b.begin());
    return std::string(b.begin(), std::min(b.end(), m.second+1));
  }

private: // functions
  void addEntry(const std::string& s, index_type i)
  {
    entries.push_back({uint32_t(keyContentArea.size()), i});
    keyContentArea.insert(keyContentArea.end(), s.begin(), s.end());
    keyContentArea.push_back('\0');
  }

private: // types
  typedef std::vector<char> KeyContentArea;

  struct Entry
  {
    // This is mostly a truncated version of a key from the input sequence.
    // The exceptions are
    //   - the first item
    //   - the last item
    //   - keys that differ from their preceding key only in the last character
    //
    // std::string pseudoKey; // std::string has too much memory overhead.
    uint32_t pseudoKeyOffset; // Instead we densely pack the key contents
                              // into keyContentArea and store in the entry
                              // the offset into that container.

    // This represents the index of the item in the input sequence right
    // after which pseudoKey might be inserted without breaking the sequence
    // order. In other words, the condition
    //
    //    sequence[lindex] <= pseudoKey <= sequence[lindex+1]
    //
    // must be true.
    index_type  lindex;
  };

  struct LookupPred
  {
    const KeyContentArea& keyContentArea;

    explicit LookupPred(const KeyContentArea* kca)
      : keyContentArea(*kca)
    {}

    const char* getKeyContent(const Entry& entry) const
    {
      return &keyContentArea[entry.pseudoKeyOffset];
    }

    bool operator()(const Entry& entry, const std::string& key) const
    {
      return key.compare(getKeyContent(entry)) > 0;
    }

    bool operator()(const std::string& key, const Entry& entry) const
    {
      return key.compare(getKeyContent(entry)) < 0;
    }
  };

  typedef std::vector<Entry> EntryCollection;

private: // data
  // Used to store the (shortened) keys as densely packed C-style strings
  KeyContentArea keyContentArea;

  LookupPred pred;

  EntryCollection entries;
};

} // namespace zim

#endif // ZIM_NARROWDOWN_H
