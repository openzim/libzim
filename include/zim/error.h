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

#ifndef ZIM_ERROR_H
#define ZIM_ERROR_H

#include <stdexcept>

namespace zim
{
  class ZimFileFormatError : public std::runtime_error
  {
    public:
      explicit ZimFileFormatError(const std::string& msg)
        : std::runtime_error(msg)
        { }
  };

  class InvalidType: public std::logic_error
  {
    public:
      explicit InvalidType(const std::string& msg)
        : std::logic_error(msg)
      {}
  };

  class EntryNotFound : public std::runtime_error
  {
    public:
      explicit EntryNotFound(const std::string& msg)
       : std::runtime_error(msg)
      {}
  };

  /* Exception thrown by the Creator in case of error.
   *
   * Most exceptions actually thrown are inheriting this exception.
   */
  class CreatorError : public std::runtime_error
  {
    public:
      explicit CreatorError(const std::string& message)
       : std::runtime_error(message)
      {}
  };

   /* Exception thrown when a entry cannot be added to the Creator.*/
  class InvalidEntry : public CreatorError
  {
    public:
      explicit InvalidEntry(const std::string& message)
       : CreatorError(message)
      {}
  };

  /* Exception thrown if a incoherence in the user implementation has been detected.
   *
   * Users need to implement interfaces such as:
   * - ContentProvider
   * - IndexData
   * - Item
   *
   * If a incoherence has been detected in those implementations a
   * `IncoherentImplementationError` will be thrown.
   */
  class IncoherentImplementationError : public CreatorError
  {
    public:
      explicit IncoherentImplementationError(const std::string& message)
       : CreatorError(message)
      {}
  };
}

#endif // ZIM_ERROR_H

