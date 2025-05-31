/*
 * Copyright (C) 2025 Veloman Yunkan <veloman.yunkan@gmail.com>
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

#ifndef OPENZIM_LIBZIM_SMARTPTR_H
#define OPENZIM_LIBZIM_SMARTPTR_H

#include <memory>

namespace zim
{

// ValuePtr is like a copyable std::unique_ptr that creates a copy of the
// referenced object when copy-constructed and/or copy-assigned
template<class T>
class ValuePtr : private std::unique_ptr<T>
{
  typedef std::unique_ptr<T> ImplType;

public:
  explicit ValuePtr(T* p) : ImplType(p) {}

  ValuePtr(){}
  ValuePtr(const ValuePtr& other) : ImplType(clone(other.get())) {}

  ValuePtr& operator=(const ValuePtr& other)
  {
    if ( this != &other ) {
      reset(clone(other.get()));
    }
    return *this;
  }

  using ImplType::get;
  using ImplType::reset;
  using ImplType::operator*;
  using ImplType::operator->;
  using ImplType::operator bool;

private:
  static T* clone(T* p) {
    return p != nullptr ? new T(*p) : nullptr;
  }
};

} // namespace zim

#endif  // OPENZIM_LIBZIM_SMARTPTR_H
