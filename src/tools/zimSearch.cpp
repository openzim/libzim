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
#include "../log.h"
#include "../arg.h"
#include <../../include/zim/search.h>
#include <../../include/zim/file.h>

void zimSearch(zim::Search& search)
{
    for (zim::Search::iterator it = search.begin(); it != search.end(); ++it)
    {
      std::cout << "article " << it->getIndex() << "\nscore " << it.get_score() << "\t:\t" << it->getTitle() << std::endl;
    }
}

int main(int argc, char* argv[])
{
  try
  {
    log_init();

    if (argc <= 2)
    {
      std::cerr << "usage: " << argv[0] << " [-x indexfile] zimfile searchstring\n"
                   "\n"
                   "options\n"
                   "  -x indexfile   specify indexfile\n"
                << std::endl;
      return 1;
    }

    std::string s = argv[2];
    for (int a = 3; a < argc; ++a)
    {
      s += ' ';
      s += argv[a];
    }

    zim::File zimfile(argv[1]);
    zim::Search search = zim::Search(&zimfile);
    search.set_query(s);
    zimSearch(search);
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

