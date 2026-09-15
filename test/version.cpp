/*
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

#include <zim/version.h>

#include <sstream>

#include "gtest/gtest.h"

namespace {

TEST(Version, getVersions)
{
  const auto versions = zim::getVersions();
  ASSERT_FALSE(versions.empty());

  bool foundLibzim = false;
  for (const auto& nameAndVersion : versions) {
    ASSERT_FALSE(nameAndVersion.first.empty());
    ASSERT_FALSE(nameAndVersion.second.empty());
    if (nameAndVersion.first == "libzim") {
      foundLibzim = true;
    }
  }
  ASSERT_TRUE(foundLibzim);
}

TEST(Version, printVersions)
{
  std::ostringstream out;
  zim::printVersions(out);
  const auto printed = out.str();
  ASSERT_NE(std::string::npos, printed.find("libzim"));
}

} // unnamed namespace
