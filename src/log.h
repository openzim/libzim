/*
 * Copyright (C) 2009 Tommi Maekitalo
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

#include "config.h"

// Should we keep the dependence on cxxtools logging framework?
#ifdef WITH_CXXTOOLS

#include <cxxtools/log.h>

#elif defined(LIBZIM_ENABLE_LOGGING)

#include <iostream>
#include <mutex>
#include <string>
#include <sstream>
#include <tuple>

namespace zim
{

class Logging
{
public:
  static void logIntoMemory();
  static std::string getInMemLogContent();
};

namespace LoggingImpl
{
  class DebugLog
  {
    std::ostream* const os_;
    std::unique_lock<std::mutex> lock_;

  public:
    explicit DebugLog(std::ostream* os);

    DebugLog(const DebugLog& ) = delete;
    void operator=(const DebugLog& ) = delete;

    operator bool() const { return os_ != nullptr; }

    std::ostream& newLogRequest();
  };

  DebugLog getDebugLog();
} // namespace LoggingImpl

} // namespace zim

#define log_debug(x) \
  if (auto debugLog = zim::LoggingImpl::getDebugLog()) { \
    debugLog.newLogRequest() << x << std::endl; \
  } else {}

// Below logging macros are not yet implemented
#define log_define(e)
#define log_fatal(e)
#define log_error(e)
#define log_warn(e)
#define log_info(e)
#define log_trace(e)
#define log_init()

#else

#define log_define(e)
#define log_fatal(e)
#define log_error(e)
#define log_warn(e)
#define log_info(e)
#define log_debug(e)
#define log_trace(e)
#define log_init()

#endif
