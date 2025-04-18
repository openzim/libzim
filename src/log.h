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
  static void orchestrateConcurrentExecutionVia(const std::string& logOutput);
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

  template<class T>
  void logValue(std::ostream& out, const T& x)
  {
      out << x;
  }

  void logValue(std::ostream& out, const char* x);
  void logValue(std::ostream& out, const std::string& x);
  void logValue(std::ostream& out, bool x);

  class FunctionCallLogger
  {
    std::string returnValue_;

  public:
    ~FunctionCallLogger();

    void changeNestingLevel(int delta);

    template<class T>
    const T& setReturnValue(const T& v)
    {
      std::ostringstream ss;
      logValue(ss, v);
      returnValue_ = ss.str();
      return v;
    }
  };

  class RAIISyncLogger
  {
  public:
    ~RAIISyncLogger();
  };

  template<size_t COUNT>
  struct TupleDumper
  {
      template<class TupleType>
      static void dump(std::ostream& out, const TupleType& t)
      {
          TupleDumper<COUNT-1>::dump(out, t);
          out << ", ";
          logValue(out, std::get<COUNT-1>(t));
      }
  };

  template<>
  struct TupleDumper<1>
  {
      template<class TupleType>
      static void dump(std::ostream& out, const TupleType& t)
      {
          logValue(out, std::get<0>(t));
      }
  };

  template<>
  struct TupleDumper<0>
  {
      template<class TupleType>
      static void dump(std::ostream& out, const TupleType& t)
      {
      }
  };

  template<class... Types>
  class FuncArgs
  {
  public:
      typedef std::tuple<Types...> ImplType;
      enum { ArgCount = std::tuple_size<ImplType>::value };

  public:
      FuncArgs(const Types&... t) : impl_(t...) {}

      void dump(std::ostream& out) const
      {
          out << "(";
          TupleDumper<ArgCount>::dump(out, impl_);
          out << ")";
      }

  private:
      ImplType impl_;
  };

  template<class... Types>
  static FuncArgs<Types...> funcArgs(Types... t)
  {
      return FuncArgs<Types...>(t...);
  };

  template<class... Types>
  std::ostream& operator<<(std::ostream& out, const FuncArgs<Types...>& x)
  {
      x.dump(out);
      return out;
  }
} // namespace LoggingImpl

} // namespace zim

#define log_debug(x) \
  if (auto debugLog = zim::LoggingImpl::getDebugLog()) { \
    debugLog.newLogRequest() << x << std::endl; \
  } else {}

#define log_debug_func_call(FUNCNAME, ...) \
  zim::LoggingImpl::FunctionCallLogger functionCallLogger; \
  log_debug(FUNCNAME << zim::LoggingImpl::funcArgs(__VA_ARGS__) << " {"); \
  functionCallLogger.changeNestingLevel(+1);


#define log_debug_return_value(v) functionCallLogger.setReturnValue(v)

#define log_debug_raii_sync_statement(statement) \
  statement; \
  log_debug("entered synchronized section"); \
  zim::LoggingImpl::RAIISyncLogger raiiSyncLogger;

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
#define log_debug_func_call(FUNCNAME, ...)
#define log_debug_return_value(v) v
#define log_debug_raii_sync_statement(statement) statement
#define log_trace(e)
#define log_init()

#endif
