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

#define LIBZIM_ENABLE_LOGGING

#include "../src/log.h"
#include "../src/namedthread.h"
#include "gtest/gtest.h"

#include <mutex>

using namespace zim;

TEST(Log, inMemLog) {
  Logging::logIntoMemory();

  log_debug("abc");

  ASSERT_EQ(Logging::getInMemLogContent(), "thread#0: abc\n");

  log_debug(123 << "xyz");

  ASSERT_EQ(Logging::getInMemLogContent(), "thread#0: abc\nthread#0: 123xyz\n");

  Logging::logIntoMemory();
  log_debug("qwerty");

  ASSERT_EQ(Logging::getInMemLogContent(), "thread#0: qwerty\n");
}

TEST(Log, inMemLogInANamedThread) {
  Logging::logIntoMemory();

  NamedThread thread("producer", [](){
    log_debug("abc");

    ASSERT_EQ(Logging::getInMemLogContent(), "producer: abc\n");

    log_debug(123);

    ASSERT_EQ(Logging::getInMemLogContent(), "producer: abc\nproducer: 123\n");

    Logging::logIntoMemory();
    log_debug("qwerty");

    ASSERT_EQ(Logging::getInMemLogContent(), "producer: qwerty\n");
  });

  thread.join();
  ASSERT_EQ(Logging::getInMemLogContent(), "producer: qwerty\n");

  NamedThread thread2("consumer", [](){
    log_debug("z");
    ASSERT_EQ(Logging::getInMemLogContent(), "producer: qwerty\nconsumer: z\n");
  });

  thread2.join();
  ASSERT_EQ(Logging::getInMemLogContent(), "producer: qwerty\nconsumer: z\n");
}

#include <chrono>

void millisleep(unsigned milliseconds) {
  // Pause the current thread for a given number of milliseconds
  // in multiple 1ms shots. This gives the scheduler higher probability
  // of waking up the concurrent thread at the right time under high load.
  using namespace std::chrono_literals;

  for ( unsigned i = 0; i < milliseconds; ++i )
    std::this_thread::sleep_for(1ms);
}

TEST(Log, concurrencyOrchestration) {
  const auto oddFlow = [](){
    millisleep(10);
    log_debug("Humpty Dumpty sat on a wall.");
    millisleep(10);
    log_debug("All the king's horses and all the king's men");
  };

  const auto evenFlow = [](){
    log_debug("Humpty Dumpty had a great fall.");
    millisleep(15);
    log_debug("Couldn't put Humpty together again.");
  };

  {
    // Make sure that non-orchestrated execution produces a serialization of
    // the concurrent operations that is different from the desired one
    Logging::logIntoMemory();
    NamedThread thread1("even", evenFlow);
    NamedThread thread2(" odd", oddFlow);

    thread1.join();
    thread2.join();

    ASSERT_EQ(Logging::getInMemLogContent(),
              "even: Humpty Dumpty had a great fall.\n"
              " odd: Humpty Dumpty sat on a wall.\n"
              "even: Couldn't put Humpty together again.\n"
              " odd: All the king's horses and all the king's men\n"
    );
  }

  const std::string outputsFromVariousOtherSerializations[] = {
    " odd: Humpty Dumpty sat on a wall.\n"
    "even: Humpty Dumpty had a great fall.\n"
    " odd: All the king's horses and all the king's men\n"
    "even: Couldn't put Humpty together again.\n"
    ,
    " odd: Humpty Dumpty sat on a wall.\n"
    " odd: All the king's horses and all the king's men\n"
    "even: Humpty Dumpty had a great fall.\n"
    "even: Couldn't put Humpty together again.\n"
    ,
    "even: Humpty Dumpty had a great fall.\n"
    "even: Couldn't put Humpty together again.\n"
    " odd: Humpty Dumpty sat on a wall.\n"
    " odd: All the king's horses and all the king's men\n"
    ,
    "even: Humpty Dumpty had a great fall.\n"
    " odd: Humpty Dumpty sat on a wall.\n"
    " odd: All the king's horses and all the king's men\n"
    "even: Couldn't put Humpty together again.\n"
    ,
    " odd: Humpty Dumpty sat on a wall.\n"
    "even: Humpty Dumpty had a great fall.\n"
    "even: Couldn't put Humpty together again.\n"
    " odd: All the king's horses and all the king's men\n"
  };

  for (const auto& desiredOutput : outputsFromVariousOtherSerializations)
  {
    Logging::logIntoMemory();
    Logging::orchestrateConcurrentExecutionVia(desiredOutput);
    NamedThread thread1("even", evenFlow);
    NamedThread thread2(" odd", oddFlow);

    thread1.join();
    thread2.join();

    ASSERT_EQ(Logging::getInMemLogContent(), desiredOutput);
  }
}

int foo(const std::string& s, size_t n)
{
  log_debug_func_call("foo", s, n);
  for(size_t i = 0; i < n; ++i)
    log_debug("fooing " << s);
  return log_debug_return_value(s.size() * n);
}

void vee(bool b)
{
  log_debug_func_call("vee", b);
  if ( b ) {
    log_debug("Vee are the champignons!");
  }
}

int bar(const char* s)
{
  log_debug_func_call("bar", s);
  if ( !s ) {
    vee(true);
    vee(false);
    return log_debug_return_value(0);
  }
  log_debug("Ready!");
  const auto r1 = foo(s, 1);
  log_debug("Steady!");
  const auto r2 = foo(s, 2);
  log_debug("Go!");
  return log_debug_return_value(r1 + r2);
}

TEST(Log, nestedFunctionCalls) {
  Logging::logIntoMemory();
  log_debug("Calling bar()...");
  bar("qwerty");
  bar(nullptr);
  log_debug("That's all");
  ASSERT_EQ(Logging::getInMemLogContent(),
R"(thread#0: Calling bar()...
thread#0: bar("qwerty") {
thread#0:  Ready!
thread#0:  foo("qwerty", 1) {
thread#0:   fooing qwerty
thread#0:  } (return value: 6)
thread#0:  Steady!
thread#0:  foo("qwerty", 2) {
thread#0:   fooing qwerty
thread#0:   fooing qwerty
thread#0:  } (return value: 12)
thread#0:  Go!
thread#0: } (return value: 18)
thread#0: bar(nullptr) {
thread#0:  vee(true) {
thread#0:   Vee are the champignons!
thread#0:  }
thread#0:  vee(false) {
thread#0:  }
thread#0: } (return value: 0)
thread#0: That's all
)");
}

TEST(Log, nestedFunctionCallsMultithreaded) {
  Logging::logIntoMemory();
  const std::string targetOutput = R"(thread#2: Calling bar(qwerty)...
thread#1: Calling bar(asdf)...
thread#1: bar("asdf") {
thread#2: bar("qwerty") {
thread#1:  Ready!
thread#2:  Ready!
thread#2:  foo("qwerty", 1) {
thread#2:   fooing qwerty
thread#1:  foo("asdf", 1) {
thread#2:  } (return value: 6)
thread#1:   fooing asdf
thread#1:  } (return value: 4)
thread#2:  Steady!
thread#1:  Steady!
thread#1:  foo("asdf", 2) {
thread#1:   fooing asdf
thread#1:   fooing asdf
thread#1:  } (return value: 8)
thread#1:  Go!
thread#2:  foo("qwerty", 2) {
thread#2:   fooing qwerty
thread#2:   fooing qwerty
thread#2:  } (return value: 12)
thread#1: } (return value: 12)
thread#1: That's all
thread#2:  Go!
thread#2: } (return value: 18)
thread#2: Done
)";

  Logging::orchestrateConcurrentExecutionVia(targetOutput);

  NamedThread thread1("thread#1", []() {
    log_debug("Calling bar(asdf)...");
    bar("asdf");
    log_debug("That's all");
  });

  NamedThread thread2("thread#2", []() {
    log_debug("Calling bar(qwerty)...");
    bar("qwerty");
    log_debug("Done");
  });

  thread1.join();
  thread2.join();

  ASSERT_EQ(Logging::getInMemLogContent(), targetOutput);
}

TEST(Log, log_debug_raii_sync_statement) {
  Logging::logIntoMemory();
  log_debug("Is there anyone in the bathroom?");
  {
    std::mutex mutex;
    log_debug_raii_sync_statement(std::unique_lock<std::mutex> lock(mutex));
    log_debug("Taking shower...");
  }
  log_debug("Did you expect something else?");
  log_debug("When I say 'bathroom', I mean 'bathroom'.");
  log_debug("FYI, I know the word 'toilet' and use it if needed.");

  ASSERT_EQ(Logging::getInMemLogContent(),
R"(thread#0: Is there anyone in the bathroom?
thread#0: entered synchronized section
thread#0: Taking shower...
thread#0: exiting synchronized section
thread#0: Did you expect something else?
thread#0: When I say 'bathroom', I mean 'bathroom'.
thread#0: FYI, I know the word 'toilet' and use it if needed.
)");
}
