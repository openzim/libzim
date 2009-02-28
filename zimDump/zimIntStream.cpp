/*
 * Copyright (C) 2007 Tommi Maekitalo
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
#include <zim/zintstream.h>
#include <cxxtools/loginit.h>
#include <cxxtools/arg.h>

log_define("zintstream");

void doDecompress(std::istream& in)
{
  unsigned col = 0;

  zim::IZIntStream z(std::cin);
  unsigned n;
  while (z.get(n))
  {
    std::cout << n;
    if (col++ >= 10)
    {
      std::cout << std::endl;
      col = 0;
    }
    else
      std::cout << ' ';
  }
  std::cout << std::endl;
}

void doCompress(std::istream& in)
{
}

int main(int argc, char* argv[])
{
  try
  {
    log_init();

    cxxtools::Arg<bool> compress(argc, argv, 'c');

    if (compress)
    {
      zim::OZIntStream z(std::cout);
      for (int a = 1; a < argc; ++a)
      {
        std::istringstream s(argv[a]);
        unsigned n;
        while (s >> n)
        {
          log_debug("compress " << n);
          z.put(n);
        }
      }
    }
    else
    {
      doDecompress(std::cin);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

