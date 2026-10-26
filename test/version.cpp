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
#include <zim/tools.h>

#include <sstream>

#include "config.h"
#include <zstd.h>
#include <lzma.h>

#if defined(ENABLE_XAPIAN)
#include <xapian.h>
#include <unicode/uversion.h>
#endif

#include "gtest/gtest.h"

namespace {

// Mirrors src/version.cpp's getVersions() so tests check real content,
// not just that a name shows up somewhere in it.
zim::LibVersions expectedVersions()
{
  zim::LibVersions versions = {
    { "libzim",  LIBZIM_VERSION      },
    { "libzstd", ZSTD_VERSION_STRING },
    { "liblzma", LZMA_VERSION_STRING }
  };

#if defined(ENABLE_XAPIAN)
  versions.push_back({ "libxapian", XAPIAN_VERSION });
  versions.push_back({"libicu", zim::Formatter() << U_ICU_VERSION_MAJOR_NUM << "."
                                            << U_ICU_VERSION_MINOR_NUM << "."
                                            << U_ICU_VERSION_PATCHLEVEL_NUM});
#endif

  return versions;
}

TEST(Version, getVersions)
{
  ASSERT_EQ(expectedVersions(), zim::getVersions());
}

TEST(Version, printVersions)
{
  const auto versions = expectedVersions();
  std::ostringstream expected;
  for (const auto& iter : versions) {
    expected << (iter != versions.front() ? "+ " : "") <<
      iter.first << " " << iter.second << std::endl;
  }

  std::ostringstream out;
  zim::printVersions(out);
  ASSERT_EQ(expected.str(), out.str());
}

} // unnamed namespace
