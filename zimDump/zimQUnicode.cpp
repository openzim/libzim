/*
 * Copyright (C) 2008 Tommi Maekitalo
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

#include <iostream>
#include <sstream>
#include <cxxtools/arg.h>
#include <zim/qunicode.h>

int main(int argc, char* argv[])
{
  try
  {
    cxxtools::Arg<bool> decode(argc, argv, 'd');

    std::ostringstream s;
    s << std::cin.rdbuf();

    if (decode)
    {
      // recode qunicode => utf-8
      std::cout << zim::QUnicodeString(s.str()).toUtf8();
    }
    else
    {
      // recode utf-8 => qunicode
      std::cout << zim::QUnicodeString::fromUtf8(s.str()).getValue();
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

