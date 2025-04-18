/*
 * Copyright (C) 2025 Veloman Yunkan
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

#ifndef OPENZIM_LIBZIM_NAMEDTHREAD_H
#define OPENZIM_LIBZIM_NAMEDTHREAD_H

#include <string>
#include <thread>

namespace zim
{

class NamedThread
{
private:
  explicit NamedThread(const std::string& name);

public:
  template <class F, class... Args>
  NamedThread(const std::string& name, F&& f, Args&&... args)
    : NamedThread(name)
  {
    thread_ = std::thread(std::forward<F>(f), std::forward<Args>(args)...);
  }

  ~NamedThread();

  NamedThread(const NamedThread& ) = delete;
  void operator=(const NamedThread& ) = delete;

  void join();

  static std::string getCurrentThreadName();

private:
  const std::string name_;
  std::thread thread_;
};

} // namespace zim

#endif  // OPENZIM_LIBZIM_NAMEDTHREAD_H
