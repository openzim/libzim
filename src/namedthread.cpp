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

#include "namedthread.h"

#include <algorithm>
#include <map>
#include <mutex>
#include <sstream>
#include <vector>

namespace zim
{

namespace
{

size_t threadCounter_ = 0;
std::vector<const NamedThread*> namedThreads_;
std::map<std::thread::id, std::string> threadId2NameMap_;

} // unnamed namespace

std::mutex NamedThread::mutex_;

NamedThread::NamedThread(const std::string& name)
  : name_(name)
{
  std::lock_guard<std::mutex> lock(mutex_);
  namedThreads_.push_back(this);
}

NamedThread::~NamedThread()
{
  join();

  std::lock_guard<std::mutex> lock(mutex_);
  const auto it = std::find(namedThreads_.begin(), namedThreads_.end(), this);
  namedThreads_.erase(it);
}

void NamedThread::join()
{
  if ( thread_.joinable() ) {
    const auto threadId = thread_.get_id();
    thread_.join();
    std::lock_guard<std::mutex> lock(mutex_);
    threadId2NameMap_.erase(threadId);
  }
}

std::string NamedThread::getCurrentThreadName()
{
  std::lock_guard<std::mutex> lock(mutex_);

  const auto curThreadId = std::this_thread::get_id();
  const auto it = threadId2NameMap_.find(curThreadId);
  if ( it != threadId2NameMap_.end() )
    return it->second;

  for (const auto nt : namedThreads_) {
    if ( nt->thread_.get_id() == curThreadId ) {
      threadId2NameMap_[curThreadId] = nt->name_;
      return nt->name_;
    }
  }

  std::ostringstream newEntryName;
  newEntryName << "thread#" << threadCounter_++;
  threadId2NameMap_[curThreadId] = newEntryName.str();
  return newEntryName.str();
}

} // namespace zim
