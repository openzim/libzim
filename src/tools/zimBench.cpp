/*
 * Copyright (C) 2009 Tommi Maekitalo
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
#include <vector>
#include <set>

#include <stdlib.h>
#include <time.h>

#include <zim/file.h>
#include <zim/fileiterator.h>
#include <zim/article.h>
#include <zim/blob.h>

#include <cxxtools/loginit.h>
#include <cxxtools/arg.h>
#include <cxxtools/clock.h>

log_define("zim.bench")

std::string randomUrl()
{
  std::string url;
  for (unsigned n = 0; n < 10; ++n)
    url += static_cast<char>('A' + rand() % 26);
  return url;
}

int main(int argc, char* argv[])
{
  try
  {
    log_init();

    cxxtools::Arg<unsigned> count(argc, argv, 'n', 1000);   // number of linear accessed articles
    cxxtools::Arg<unsigned> randomCount(argc, argv, 'r', count);  // number of random accesses
    cxxtools::Arg<unsigned> distinctCount(argc, argv, 'd', randomCount);  // number of distinct articles used for random access
    cxxtools::Arg<char> ns(argc, argv, "--ns", 'A');

    if (argc != 2)
    {
      std::cerr << "usage: " << argv[0] << " [options] zimfile\n"
                   "\t-n number\tnumber of linear accessed articles (default 1000)\n"
                   "\t-r number\tnumber of random accessed articles (default: same as -n)\n"
                   "\t-d number\tnumber of distinct articles used for random access (default: same as -r)\n"
                << std::flush;
      return 1;
    }

    srand(time(0));

    std::string filename = argv[1];

    std::cout << "open file " << filename << std::endl;
    zim::File file(filename);

    // collect urls
    typedef std::set<std::string> UrlsType;
    typedef std::vector<std::string> RandomUrlsType;
    UrlsType urls;
    RandomUrlsType randomUrls;

    std::cout << "collect linear urls" << std::endl;
    for (zim::File::const_iterator it = file.begin(); it != file.end() && urls.size() < count; ++it)
    {
      log_debug("check url " << it->getUrl() << '\t' << urls.size() << " found");
      if (!it->isRedirect())
        urls.insert(it->getUrl());
    }

    std::cout << urls.size() << " urls collected" << std::endl;

    std::cout << "collect random urls" << std::endl;
    while (randomUrls.size() < distinctCount)
    {
      zim::File::const_iterator it = file.find(ns, randomUrl());
      log_debug("check url " << it->getUrl() << '\t' << randomUrls.size() << " found");
      if (!it->isRedirect())
        randomUrls.push_back(it->getUrl());
    }

    std::cout << randomUrls.size() << " random urls collected" << std::endl;

    // reopen file
    file = zim::File();
    file = zim::File(filename);

    // linear read
    std::cout << "linear:" << std::flush;
    cxxtools::Clock clock;
    clock.start();

    unsigned size = 0;
    for (UrlsType::const_iterator it = urls.begin(); it != urls.end(); ++it)
      size += file.getArticle(ns, *it).getData().size();

    cxxtools::Timespan t = clock.stop();
    std::cout << "\tsize=" << size << "\tt=" << (t.totalMSecs() / 1000.0) << "s\t" << (static_cast<double>(urls.size()) / t.totalMSecs() * 1000.0) << " articles/s" << std::endl;

    // reopen file
    file = zim::File();
    file = zim::File(filename);

    // random access
    std::cout << "random:" << std::flush;
    clock.start();

    size = 0;
    for (unsigned r = 0; r < randomCount; ++r)
      size += file.getArticle(ns, randomUrls[rand() % randomUrls.size()]).getData().size();

    //for (UrlsType::const_iterator it = randomUrls.begin(); it != randomUrls.end(); ++it)
      //size += file.getArticle(ns, *it).getData().size();

    t = clock.stop();
    std::cout << "\tsize=" << size << "\tt=" << (t.totalMSecs() / 1000.0) << "s\t" << (static_cast<double>(randomCount) / t.totalMSecs() * 1000.0) << " articles/s" << std::endl;
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

