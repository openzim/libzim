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

#ifndef DEBUG_H_
#define DEBUG_H_

#include <iostream>
#include <stdlib.h>

#if defined (NDEBUG)
# define ASSERT(left, operator, right) (void(0))
#else

#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__ANDROID__)
#include <execinfo.h>
#endif

template<typename T, typename U>
void _on_assert_fail(const char* vara, const char* op, const char* varb,
                     T a, U b, const char* file, int line)  {
  std::cerr << "\nAssertion failed at "<< file << ":" << line << "\n " <<
      vara << "[" << a << "] " << op << " " << varb << "[" << b << "]" <<
      std::endl;

#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__ANDROID__)
  void *callstack[64];
  size_t size;
  size = backtrace(callstack, 64);
  char** strings = backtrace_symbols(callstack, size);
  for (size_t i=0; i<size; i++) {
    std::cerr << strings[i] << std::endl;
  }
  free(strings);
#endif
  exit(1);
}

# define ASSERT(left, operator, right) do { auto _left = left; auto _right = right; if (!((_left) operator (_right))) _on_assert_fail(#left, #operator, #right, _left, _right, __FILE__, __LINE__);  } while(0)

#endif

#endif
