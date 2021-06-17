/*
 * Copyright 2013-2016 Emmanuel Engelhart <kelson@kiwix.org>
 * Copyright 2016 Matthieu Gautier <mgautier@kymeria.fr>
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

#include <string>
#include <tuple>
#include "config.h"

namespace zim {
  bool isCompressibleMimetype(const std::string& mimetype);
#if defined(ENABLE_XAPIAN)
  std::string removeAccents(const std::string& text);
#endif
  uint32_t countWords(const std::string& text);
  void microsleep(int microseconds);

  std::tuple<char, std::string> parseLongPath(const std::string& longPath);

  // Parse a illustration path ("Illustration_<width>x<height>@1") to a size.
  unsigned int parseIllustrationPathToSize(const std::string& s);

  /** Return a random number from range [0, max]
   *
   * This function is threadsafe
   **/
  uint32_t randomNumber(uint32_t max);
}

#endif  // OPENZIM_LIBZIM_TOOLS_H
