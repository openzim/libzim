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

#include <cstddef>
#include <string>
#include <tuple>
#include <map>
#include <vector>
#include "config.h"

#include <zim/item.h>

#if defined(ENABLE_XAPIAN)
namespace Xapian {
  class Database;
  class Stem;
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

  std::string LIBZIM_PRIVATE_API stripMimeParameters(const std::string& rawMimeType);

  /** Maximum size (in bytes) of a redirect fragment that can be stored.
   *
   * The dirent "parameter" field this is encoded into is itself limited to
   * 255 bytes on disk; MAX_REDIRECT_FRAGMENT_SIZE is that limit minus the
   * 2-byte record header encodeRedirectParameter() adds.
   */
  constexpr size_t MAX_REDIRECT_FRAGMENT_SIZE = 253;

  /** Encode a redirect's optional fragment into dirent "parameter" bytes.
   *
   * The dirent "parameter" field is a small (<= 255 bytes), currently
   * unused-by-anything-else, per-entry byte string reserved by the ZIM
   * format for this kind of extension. To leave room for other, unrelated
   * data that may be stored there in the future, the fragment is not
   * written as a bare string but as a single tagged record:
   *
   *   1 byte    record tag, always FRAGMENT_PARAM_TAG for now
   *   1 byte    N, the length of the fragment in bytes
   *   N bytes   the fragment itself, exactly as given (UTF-8, no leading
   *             '#', not URL-encoded)
   *
   * A redirect without a fragment is encoded as an empty string, identical
   * to the on-disk representation redirects have always had -- this changes
   * nothing for existing ZIM files or for redirects that don't use it.
   *
   * @param fragment The fragment to encode, without a leading '#'.
   * @return The bytes to store in the dirent's "parameter" field.
   * @exception std::invalid_argument if fragment is longer than
   *            MAX_REDIRECT_FRAGMENT_SIZE bytes.
   */
  std::string LIBZIM_PRIVATE_API encodeRedirectParameter(const std::string& fragment);

  /** Decode a redirect's optional fragment from dirent "parameter" bytes.
   *
   * Inverse of encodeRedirectParameter(). Never throws: a "parameter" value
   * this function doesn't recognize (empty, truncated, or produced by some
   * future record type this version of libzim doesn't know about) decodes
   * to "", the same as a redirect with no fragment at all. Reading a ZIM
   * file must never fail just because a later library version stored
   * something in this field that an older reader doesn't understand.
   *
   * @param parameter The raw bytes of a redirect dirent's "parameter" field.
   * @return The decoded fragment (without a leading '#'), or "" if none.
   */
  std::string LIBZIM_PRIVATE_API decodeRedirectFragment(const std::string& parameter);

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

// Xapian based tools
#if defined(ENABLE_XAPIAN)
  std::string LIBZIM_PRIVATE_API removeAccents(const std::string& text);
  bool LIBZIM_PRIVATE_API getDbFromAccessInfo(zim::ItemDataDirectAccessInfo accessInfo, Xapian::Database& database);
  Xapian::Stem getXapianStemmer(const std::string& iso639LangCode);
#endif
}

#endif  // OPENZIM_LIBZIM_TOOLS_H
