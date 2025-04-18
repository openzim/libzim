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

// Compile this translation unit as if logging has been enabled
// so that if it is enabled in any other translation unit the linking
// step doesn't end with an undefined symbol error.
#define LIBZIM_ENABLE_LOGGING

#include "log.h"

#include <condition_variable>
#include <iomanip>
#include <sstream>

#include "namedthread.h"
#include "tools.h"

namespace zim
{

namespace
{

std::mutex mutex_;

// When non-empty, the last element represents the name of the thread
// that should be allowed to proceed for the next logging statement.
std::vector<std::string> orchestrationStack_;

std::condition_variable cv_;

bool threadMayProceed(const std::string& threadName)
{
  return orchestrationStack_.empty()
      || threadName == orchestrationStack_.back();
}

} // unnamed namespace

namespace LoggingImpl
{

DebugLog::DebugLog(std::ostream* os)
  : os_(os)
  , lock_(mutex_)
{
}

std::ostream& DebugLog::newLogRequest()
{
  const auto threadName = NamedThread::getCurrentThreadName();
  if ( !threadMayProceed(threadName) ) {
    cv_.wait(lock_, [threadName]() { return threadMayProceed(threadName); });
  }

  *os_ << threadName << ": ";

  if ( !orchestrationStack_.empty() ) {
    orchestrationStack_.pop_back();
    cv_.notify_all();
  }

  return *os_;
}

} // namespace LoggingImpl

void Logging::orchestrateConcurrentExecutionVia(const std::string& logOutput)
{
  orchestrationStack_.clear();
  for ( auto line : zim::split(logOutput, "\n") ) {
    const std::string threadName = zim::split(line, ":")[0];
    orchestrationStack_.push_back(threadName);
  }
  std::reverse(orchestrationStack_.begin(), orchestrationStack_.end());
}

namespace
{
std::ostream* logStreamPtr_;
std::ostringstream inMemLog_;
} // unnamed namespace

namespace LoggingImpl
{

DebugLog getDebugLog()
{
  return DebugLog(logStreamPtr_);
}

} // namespace LoggingImpl

void Logging::logIntoMemory()
{
  inMemLog_.str("");
  logStreamPtr_ = &inMemLog_;
}

std::string Logging::getInMemLogContent()
{
  return inMemLog_.str();
}

} // namespace zim
