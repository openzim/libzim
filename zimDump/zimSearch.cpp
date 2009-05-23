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
#include <cxxtools/loginit.h>
#include <cxxtools/arg.h>
#include <zim/search.h>

void zimSearch(zim::Search& search, const std::string& s)
{
    zim::Search::Results result;
    search.search(result, s);

    for (zim::Search::Results::const_iterator it = result.begin(); it != result.end(); ++it)
    {
      std::cout << "article " << it->getArticle().getIndex() << "\tpriority " << it->getPriority() << "\t:\t" << it->getArticle().getTitle() << std::endl;
    }
}

int main(int argc, char* argv[])
{
  try
  {
    log_init();

    cxxtools::Arg<std::string> indexfile(argc, argv, 'x');

    cxxtools::Arg<double> weightOcc(argc, argv, "--weight-occ");
    cxxtools::Arg<double> weightOccOff(argc, argv, "--weight-occ-off");
    cxxtools::Arg<double> weightPlus(argc, argv, "--weight-plus");
    cxxtools::Arg<double> weightDist(argc, argv, "--weight-dist");
    cxxtools::Arg<double> weightPos(argc, argv, "--weight-pos");
    cxxtools::Arg<double> weightPosRel(argc, argv, "--weight-pos-rel");
    cxxtools::Arg<double> weightDistinctWords(argc, argv, "--weight-distinct-words");

    if (weightOcc.isSet())
      zim::Search::setWeightOcc(weightOcc);
    if (weightOccOff.isSet())
      zim::Search::setWeightOccOff(weightOccOff);
    if (weightPlus.isSet())
      zim::Search::setWeightPlus(weightPlus);
    if (weightDist.isSet())
      zim::Search::setWeightDist(weightDist);
    if (weightPos.isSet())
      zim::Search::setWeightPos(weightPos);
    if (weightPosRel.isSet())
      zim::Search::setWeightPosRel(weightPosRel);
    if (weightDistinctWords.isSet())
      zim::Search::setWeightDistinctWords(weightDistinctWords);

    if (argc <= 2)
    {
      std::cerr << "usage: " << argv[0] << " [-x indexfile] zimfile searchstring\n"
                   "\n"
                   "options\n"
                   "  -x indexfile   specify indexfile\n"
                   "options to tune search parameters:)\n"
                   "  --weight-occ number (default " << zim::Search::getWeightOcc() << ")\n"
                   "  --weight-occ-off number (default " << zim::Search::getWeightOccOff() << ")\n"
                   "  --weight-plus number (default " << zim::Search::getWeightPlus() << ")\n"
                   "  --weight-dist number (default " << zim::Search::getWeightDist() << ")\n"
                   "  --weight-pos number (default " << zim::Search::getWeightPos() << ")\n"
                   "  --weight-pos-rel number (default " << zim::Search::getWeightPosRel() << ")\n"
                   "  --weight-distinct-words number (default " << zim::Search::getWeightDistinctWords() << ')'
                << std::endl;
      return 1;
    }

    std::string s = argv[2];
    for (int a = 3; a < argc; ++a)
    {
      s += ' ';
      s += argv[a];
    }

    if (indexfile.isSet())
    {
      zim::Search search = zim::Search(zim::File(argv[1]), zim::File(indexfile));
      zimSearch(search, s);
    }
    else
    {
      zim::Search search = zim::Search(zim::File(argv[1]));
      zimSearch(search, s);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

