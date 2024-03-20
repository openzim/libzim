/*
 * Copyright (C) 2022 Matthieu Gautier <mgautier@kymeria.fr>
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

#ifndef ZIM_TOOLS_H
#define ZIM_TOOLS_H

#include "zim.h"
#include <sstream>
namespace zim {
#if defined(LIBZIM_WITH_XAPIAN)

  /** Helper function to set the icu data directory.
   *
   * On Android, we compile ICU without data integrated
   * in the library. So android application needs to set
   * the data directory where ICU can find its data.
   */
  LIBZIM_API void setICUDataDirectory(const std::string& path);

#endif

  /**
   * @brief Stringstream Class to use itself as the stream object 
   * returned by << operator. (std::stringstream returns an std::ostream). 
   * Allows a one-line stringstream to str conversion, e.g. use_str(Formatter()
   * << "foo" << variable);
   *
   */
  class Formatter
  {
  public:
    Formatter() {}
    ~Formatter() {}

    template <typename Type> Formatter &operator<<(const Type &value)
    {
      stream_ << value;
      return *this;
    }

    /* Operator for function templates like std::endl */
    Formatter &operator<<(std::ostream& (* __pf)(std::ostream&))
    {
      stream_ << __pf;
      return *this;
    }

    /* Operator for working with other ostream like std::cerr */
    friend std::ostream &operator<<(std::ostream &os, const Formatter &obj)
    {
      os << obj.stream_.str();
      return os;
    }

    operator std::string() const { return stream_.str(); }

  private:
    /* Disable copy and assignment constructors */
    Formatter(const Formatter &) = delete;
    Formatter &operator=(Formatter &) = delete;

    /* Simple composition with std::stringstream */
    std::stringstream stream_;
  };
}

#endif // ZIM_TOOLS_H
