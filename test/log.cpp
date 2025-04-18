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

TEST(Log, concurrencyOrchestration) {
  using namespace std::chrono_literals;

  const auto oddFlow = [](){
    std::this_thread::sleep_for(10ms);
    log_debug("Humpty Dumpty sat on a wall.");
    std::this_thread::sleep_for(10ms);
    log_debug("All the king's horses and all the king's men");
  };

  const auto evenFlow = [](){
    log_debug("Humpty Dumpty had a great fall.");
    std::this_thread::sleep_for(15ms);
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
