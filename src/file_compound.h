/*
 * Copyright (C) 2020-2021 Veloman Yunkan
 * Copyright (C) 2017-2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#ifndef ZIM_FILE_COMPOUND_H_
#define ZIM_FILE_COMPOUND_H_

#include "file_part.h"
#include "zim_types.h"
#include "debug.h"
#include "config.h"

#include <map>
#include <memory>
#include <vector>

namespace zim {

struct Range {
  Range(const offset_t  min, const offset_t max)
    : min(min), max(max)
  {
      // ASSERT(min, <, max);
  }

  const offset_t min;
  const offset_t max;
};

struct less_range
{
  bool operator()(const Range& lhs, const Range& rhs) const {
    return lhs.min < rhs.min && lhs.max <= rhs.min;
  }
};

class LIBZIM_PRIVATE_API FileCompound : private std::map<Range, FilePart*, less_range> {
    typedef std::map<Range, FilePart*, less_range> ImplType;

  public: // types
    typedef const_iterator PartIterator;
    typedef std::pair<PartIterator, PartIterator> PartRange;
    enum class MultiPartToken { Multi };

  public: // functions
    static std::shared_ptr<FileCompound> openSinglePieceOrSplitZimFile(const std::string& filename);
    explicit FileCompound(const std::string& filename);
    explicit FileCompound(const std::string& filename, MultiPartToken token);

#ifndef _WIN32
    explicit FileCompound(int fd);
    explicit FileCompound(FdInput fd);
    explicit FileCompound(const std::vector<FdInput>& fds);
#endif

    ~FileCompound();

    using ImplType::begin;
    using ImplType::end;

    const std::string& filename() const { return _filename; }
    zsize_t fsize() const { return _fsize; };
    time_t getMTime() const;
    bool fail() const { return empty(); };
    bool is_multiPart() const { return size() > 1; };

    PartIterator locate(offset_t offset) const {
      const PartIterator partIt = lower_bound(Range(offset, offset));
      ASSERT(partIt != end(), ==, true);
      return partIt;
    }

    PartRange locate(offset_t offset, zsize_t size) const {
      const Range queryRange(offset, offset+size);
      // equal_range expects comparator to satisfy the `Compare` requirement.
      // (ie `comp(a, b) == !comp(b, a)`) which is not the case for `less_range`
      // If not satisfy, this is UB.
      // Even if UB, stdlib's equal_range behaves "correctly".
      // But libc++ (used in Apple, Android, ..) is not.
      // In all case, we are triggering a UB and it is to us to not call equal_range.
      // So let's use lower_bound and upper_bound which doesn't need such requirement.
      // See https://stackoverflow.com/questions/67042750/should-setequal-range-return-pair-setlower-bound-setupper-bound
      return {lower_bound(queryRange), upper_bound(queryRange)};
    }

  private: // functions
    void addPart(FilePart* fpart);

  private: // data
    std::string _filename;
    zsize_t _fsize;
    mutable time_t mtime;
};


};


#endif //ZIM_FILE_COMPOUND_H_
