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
#include "gtest/gtest.h"

using namespace zim;

TEST(Log, inMemLog) {
  Logging::logIntoMemory();

  log_debug("abc");

  ASSERT_EQ(Logging::getInMemLogContent(), "abc\n");

  log_debug(123 << "xyz");

  ASSERT_EQ(Logging::getInMemLogContent(), "abc\n123xyz\n");

  Logging::logIntoMemory();
  log_debug("qwerty");

  ASSERT_EQ(Logging::getInMemLogContent(), "qwerty\n");
}
