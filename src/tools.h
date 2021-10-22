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
#include <map>
#include <vector>
#include "config.h"

#include <zim/item.h>

#if defined(ENABLE_XAPIAN)
namespace Xapian {
  class Database;
}
#endif  // ENABLE_XAPIAN
namespace zim {
  bool isCompressibleMimetype(const std::string& mimetype);
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

  std::vector<std::string> split(const std::string & str,
                                const std::string & delims=" *-");

  std::map<std::string, int> read_valuesmap(const std::string& s);

// Xapian based tools
#if defined(ENABLE_XAPIAN)
  std::string removeAccents(const std::string& text);
  bool getDbFromAccessInfo(Item::DirectAccessInfo accessInfo, Xapian::Database& database);
#endif
}

#endif  // OPENZIM_LIBZIM_TOOLS_H

/* Formatter for std::exception what() message:
 * throw std::runtime_error(
 *   Formatter() << "zimwriterfs: Unable to read" << filename << ": " << strerror(errno));
 */
class Formatter
{
public:
  Formatter() {}
  ~Formatter() {}
  
  template <typename Type>
  Formatter & operator << (const Type & value)
  {
    stream_ << value;
    return *this;
  }
  
  operator std::string () const   { return stream_.str(); }
  
private:
  Formatter(const Formatter &);
  Formatter & operator = (Formatter &);
  
  std::stringstream stream_;
};


