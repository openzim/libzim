/*
 * Copyright (C) 2017 Matthieu Gautier <mgautier@kymeria.fr>
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

#include <zim/file_part.h>
#include <map>
#include <memory>

namespace zim {

class FileReader;

struct Range {
  Range(const size_t point ) : min(point), max(point) {}
  Range(const size_t min, const size_t max) : min(min), max(max) {}
  const size_t min;
  const size_t max;
};

struct less_range : public std::binary_function< Range, Range, bool>
{
  bool operator()(const Range& lhs, const Range& rhs) const {
    return lhs.min < rhs.min && lhs.max <= rhs.min;
  }
};

class FileCompound : public std::map<Range, FilePart*, less_range> {
  public:
    FileCompound(const std::string& filename);

    size_t fsize() const { return _fsize; };
    time_t getMTime() const;
    bool fail() const { return empty(); };
    bool is_multiPart() const { return size() > 1; };

  private:
    size_t _fsize;
    mutable time_t mtime;
};


};


#endif //ZIM_FILE_COMPOUND_H_
