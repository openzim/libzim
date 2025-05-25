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

#include <mutex>
#include <string>
#include <thread>

#include "config.h"

namespace zim
{

class LIBZIM_PRIVATE_API NamedThread
{
private:
  explicit NamedThread(const std::string& name);

public:
  template <class F>
  NamedThread(const std::string& name, F&& f)
    : NamedThread(name)
  {
    // Ensure that f starts executing after the assignment to
    // the thread_ data member has completed (so that any possible
    // calls to NamedThread::getCurrentThreadName() from inside f()
    // read the correct value of thread id).
    std::mutex& mutex = getMutex();
    std::lock_guard<std::mutex> lock(mutex);

    thread_ = std::thread([f, &mutex]() { mutex.lock(); mutex.unlock(); f(); });
  }

  ~NamedThread();

  NamedThread(const NamedThread& ) = delete;
  void operator=(const NamedThread& ) = delete;

  void join();

  static std::string getCurrentThreadName();

private: // functions
  // This is a workaround for a bug in our build system that prevents
  // LIBZIM_PRIVATE_API and/or LIBZIM_API classes from having static data
  // members
  static std::mutex& getMutex();

private: // data
  const std::string name_;
  std::thread thread_;
};

} // namespace zim

#endif  // OPENZIM_LIBZIM_NAMEDTHREAD_H
