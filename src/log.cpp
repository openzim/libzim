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

#include <iomanip>
#include <sstream>

#include "namedthread.h"
#include "tools.h"

namespace zim
{

namespace
{

std::mutex mutex_;

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

  *os_ << threadName << ": ";

  return *os_;
}

} // namespace LoggingImpl

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
