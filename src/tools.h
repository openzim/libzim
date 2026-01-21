/*
 * Copyright (C) 2016-2020 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2021 Maneesh P M <manu.pm55@gmail.com>
 * Copyright (C) 2013-2016 Emmanuel Engelhart <kelson@kiwix.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU  General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef OPENZIM_LIBZIM_TOOLS_H
#define OPENZIM_LIBZIM_TOOLS_H

#include <cstddef> // size_t
#include <cstring> // memcmp
#include <string>
#include <tuple>
#include <map>
#include <vector>
#include "config.h"

#include <zim/item.h>
#include "zim/writer/item.h"

#if defined(ENABLE_XAPIAN)
namespace Xapian {
  class Database;
}
#endif  // ENABLE_XAPIAN
namespace zim {
  bool isCompressibleMimetype(const std::string& mimetype);
  uint32_t LIBZIM_PRIVATE_API countWords(const std::string& text);
  void LIBZIM_PRIVATE_API microsleep(int microseconds);

  std::tuple<char, std::string> LIBZIM_PRIVATE_API parseLongPath(const std::string& longPath);

  // Parse a illustration path ("Illustration_<width>x<height>@1") to a size.
  unsigned int LIBZIM_PRIVATE_API parseIllustrationPathToSize(const std::string& s);

  /** Return a random number from range [0, max]
   *
   * This function is threadsafe
   **/
  uint32_t LIBZIM_PRIVATE_API randomNumber(uint32_t max);

  std::vector<std::string> split(const std::string & str,
                                const std::string & delims=" *-");

  std::map<std::string, int> read_valuesmap(const std::string& s);

  using MimeCounterType = std::map<const std::string, zim::entry_index_type>;
  MimeCounterType LIBZIM_PRIVATE_API parseMimetypeCounter(const std::string& counterData);

  template<class Filter>
  entry_index_type countMimeType(const std::string& counterData, Filter filter) {
    entry_index_type count = 0;
    for (auto& pair: parseMimetypeCounter(counterData)) {
      if (filter(pair.first)) {
        count += pair.second;
      }
    }
    return count;
  }

  // Work around for errors reported by too picky UB sanitizer tools on std::memcmp()
  // being called with a nullptr pointer argument even though count==0. Explicit
  // nullptr checks to satisfy other picky compilers.
  template <typename T>
  int safe_memcmp(T const *const lhs, T const *const rhs, std::size_t count) noexcept {
      if(count == 0UL || lhs == nullptr || rhs == nullptr) {
          return 0;
      }
      return std::memcmp(lhs, rhs, count);
  }

  namespace writer {
    class Dirent;
    bool isFrontArticle(const Dirent* dirent, const Hints& hints);
  }

// Xapian based tools
#if defined(ENABLE_XAPIAN)
  std::string LIBZIM_PRIVATE_API removeAccents(const std::string& text);
  bool LIBZIM_PRIVATE_API getDbFromAccessInfo(zim::ItemDataDirectAccessInfo accessInfo, Xapian::Database& database);
#endif
}

#endif  // OPENZIM_LIBZIM_TOOLS_H
