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

#include "zim/illustration.h"

#include "gtest/gtest.h"


namespace
{

TEST(Illustrations, parsingOfValidInput)
{
#define CHECK(str, parenthesized_result) \
  ASSERT_EQ(zim::IllustrationInfo::fromMetadataItemName(str), \
            zim::IllustrationInfo parenthesized_result)

  CHECK("Illustration_0x0@1",   ({  0,  0, 1.0, {} }) );
  CHECK("Illustration_1x1@1",   ({  1,  1, 1.0, {} }) );
  CHECK("Illustration_01x01@1", ({  1,  1, 1.0, {} }) );
  CHECK("Illustration_64x64@1", ({ 64, 64, 1.0, {} }) );
  CHECK("Illustration_64x64@2", ({ 64, 64, 2.0, {} }) );
  CHECK("Illustration_64x48@2", ({ 64, 48, 2.0, {} }) );
#undef CHECK
}

TEST(Illustrations, parsingOfInvalidInput)
{
#define CHECK_PARSING_OF_INVALID_INPUT(str) \
  ASSERT_THROW(zim::IllustrationInfo::fromMetadataItemName(str), \
               std::runtime_error)

  CHECK_PARSING_OF_INVALID_INPUT("Illstration_64x64@1");
  CHECK_PARSING_OF_INVALID_INPUT("Illstration_");
  CHECK_PARSING_OF_INVALID_INPUT("Illustration_64x@1");
  CHECK_PARSING_OF_INVALID_INPUT("Illustration_64x64@1;scheme=light");
  CHECK_PARSING_OF_INVALID_INPUT("Illustration_64x");
  CHECK_PARSING_OF_INVALID_INPUT("Illustration_64x64");
  CHECK_PARSING_OF_INVALID_INPUT("Illustration_-32x-32@1");
  CHECK_PARSING_OF_INVALID_INPUT("Illustration_ 64x64@1");
  CHECK_PARSING_OF_INVALID_INPUT("Illustration_64x 64@1");
  CHECK_PARSING_OF_INVALID_INPUT("Illustration_ 64x 64@1");
  CHECK_PARSING_OF_INVALID_INPUT("Illustration_1 28x1 28@1");

#undef CHECK_PARSING_OF_INVALID_INPUT
}

} // unnamed namespace
