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

log_define("zintstream")

void doDecompress(std::istream& in)
{
  unsigned col = 0;

  zim::ZIntStream z(std::cin);
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

std::string zintCompress(unsigned value)
{
  std::ostringstream s;
  zim::ZIntStream z(s);
  z.put(value);
  return s.str();
}

int main(int argc, char* argv[])
{
  try
  {
    log_init();

    cxxtools::Arg<bool> compress(argc, argv, 'c');

    if (compress)
    {
      zim::ZIntStream z(std::cout);

      if (argc > 1)
      {
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
        unsigned n;
        while (std::cin >> n)
          z.put(n);
      }
    }
    else if (argc > 1)
    {
      for (int a = 1; a < argc; ++a)
      {
        std::istringstream conv(argv[a]);
        zim::size_type number;
        conv >> number;
        if (conv)
        {
          std::ostringstream s;
          zim::ZIntStream z(s);
          z.put(number);
          std::string x = s.str();
          std::cout << number << " => " << std::hex;
          for (std::string::const_iterator it = x.begin(); it != x.end(); ++it)
            std::cout << static_cast<unsigned>(static_cast<unsigned char>(*it)) << ' ';
          std::cout << std::dec << '\n';
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

