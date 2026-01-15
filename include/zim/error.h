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

#include "zim.h"
#include "tools.h"

#include <stdexcept>
#include <typeinfo>

namespace zim
{
  class LIBZIM_API ZimFileFormatError : public std::runtime_error
  {
    public:
      explicit ZimFileFormatError(const std::string& msg)
        : std::runtime_error(msg)
        { }
  };

  class LIBZIM_API InvalidType: public std::logic_error
  {
    public:
      explicit InvalidType(const std::string& msg)
        : std::logic_error(msg)
      {}
  };

  class LIBZIM_API EntryNotFound : public std::runtime_error
  {
    public:
      explicit EntryNotFound(const std::string& msg)
       : std::runtime_error(msg)
      {}
  };

  /**
   * Exception thrown by the Creator in case of error.
   *
   * Most exceptions actually thrown are subclasses of this type.
   */
  class LIBZIM_API CreatorError : public std::runtime_error
  {
    public:
      explicit CreatorError(const std::string& message)
       : std::runtime_error(message)
      {}
  };

  /**
   * Exception thrown when an entry cannot be added to the ZIM archive.
   */
  class LIBZIM_API InvalidEntry : public CreatorError
  {
    public:
      explicit InvalidEntry(const std::string& message)
       : CreatorError(message)
      {}
  };

  /**
   * Exception thrown if incoherence in the user implementation has been
   * detected.
   *
   * Users need to implement interfaces such as:
   * - zim::writer::ContentProvider
   * - zim::writer::IndexData
   * - zim::writer::Item
   *
   * If incoherence has been detected in those implementations an
   * `IncoherentImplementationError` will be thrown.
   */
  class LIBZIM_API IncoherentImplementationError : public CreatorError
  {
    public:
      explicit IncoherentImplementationError(const std::string& message)
       : CreatorError(message)
      {}
  };

  /**
   * Exception thrown when there are problems indexing a title.
   *
   * In the current implementation, the only situation deliberately targeted by
   * this type of error is when the title appears to contain a word that
   * exceeds the limit on the longest indexable word (as defined by
   * `MAX_INDEXABLE_TITLE_WORD_SIZE` in `src/constants.h`) but - due to hacky
   * implementation - titles containing too much whitespace and/or punctuation
   * may also trigger this error.
   */
  class LIBZIM_API TitleIndexingError : public CreatorError
  {
    public:
      explicit TitleIndexingError(const std::string& message)
       : CreatorError(message)
      {}
  };

  /**
   * Exception thrown in the main thread when another exception has been
   * thrown in another worker thread.
   *
   * Creator uses different worker threads to do background work.
   * If an exception is thrown in one of this threads, it is catched and
   * "rethrown" in the main thread as soon as possible with a `AsyncError`.
   *
   * AsyncError contains the original exception. You can rethrow the original
   * exception using `rethrow`:
   *
   * ```
   * try {
   *   creator->addStuff(...);
   * } catch (const zim::AsyncError& e) {
   *   // An exception has been thrown in a worker thread
   *   try {
   *     e.rethrow();
   *   } catch (const std::exception& original_exception) {
   *     // original_exception is the exception thrown in the worker thread
   *     ...
   *   }
   * }
   * ```
   */
  class LIBZIM_API AsyncError : public CreatorError
  {
    public:
      explicit AsyncError(const std::exception_ptr exception)
       : CreatorError(buildErrorMessage(exception)),
         m_exception(exception)
      {}

      [[noreturn]] void rethrow() const {
        std::rethrow_exception(m_exception);
      }

    private: // data
      std::exception_ptr m_exception;

    private: // function
      static std::string buildErrorMessage(const std::exception_ptr exception) {
        try {
          std::rethrow_exception(exception);
        } catch (const std::exception& e) {
          return Formatter()
                 << "Asynchronous error: " << typeid(e).name() << std::endl
                 << e.what();
        } catch (...) {
          return "Unknown asynchronous exception";
        }
      }
  };

  /**
   * Exception thrown when the creator is in error state.
   *
   * If the creator is in error state (mostly because an AsyncError has already
   * been thrown), any call to any method on it will throw a
   * `CreatorStateError`.
   */
  class LIBZIM_API CreatorStateError : public CreatorError
  {
    public:
      explicit CreatorStateError()
       : CreatorError("Creator is in error state.")
      {}
  };
}

#endif // ZIM_ERROR_H

