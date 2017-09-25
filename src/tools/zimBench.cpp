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

#include "../log.h"
#include <getopt.h>

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
  int count = 1000;
  bool randomCountSet = false;
  int randomCount = 1000;
  bool distinctCountSet = false;
  int distinctCount = 1000;
  char ns = 'A';
  std::string filename;

  static struct option long_options[]
    = {{"ns", required_argument, 0, 's'}};
  try
  {
    log_init();

    while (true) {
      int option_index = 0;
      int c = getopt_long(argc, argv, "sn:r:d:",
              long_options, &option_index);

      if (c!= -1) {
        switch (c) {
          case 's':
            ns = optarg[0];
            break;
          case 'n':
            count = atoi(optarg);
            if (! randomCountSet ) {
              randomCount = count;
              if (! distinctCountSet ) {
                distinctCount = count;
              }
            }
            break;
          case 'r':
            randomCountSet = true;
            randomCount = atoi(optarg);
            if (! distinctCountSet ) {
              distinctCount = count;
            }
            break;
          case 'd':
            distinctCountSet = true;
            distinctCount = atoi(optarg);
            break;
        };
      } else {
        if (optind < argc ) {
          filename = argv[optind++];
        }
        break;
      }
    }

    if (filename.empty())
    {
      std::cerr << "usage: " << argv[0] << " [options] zimfile\n"
                   "\t-n number\tnumber of linear accessed articles (default 1000)\n"
                   "\t-r number\tnumber of random accessed articles (default: same as -n)\n"
                   "\t-d number\tnumber of distinct articles used for random access (default: same as -r)\n"
                << std::flush;
      return 1;
    }

    srand(time(0));

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
      std::cout << "check url " << it->getUrl() << '\t' << urls.size() << " found" << std::endl;
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
    struct timespec time_start;
    struct timespec time_stop;
    long long milliseconds_start, milliseconds_stop, milliseconds_delta;

    clock_gettime(CLOCK_REALTIME, &time_start);
    milliseconds_start = time_start.tv_sec*1000LL + time_start.tv_nsec/1000;

    unsigned size = 0;
    for (UrlsType::const_iterator it = urls.begin(); it != urls.end(); ++it) {
      auto article = file.getArticle(ns, *it);
      if (article.good()) {
        size += file.getArticle(ns, *it).getData().size();
      } else {
        std::cerr << "Impossible to get article '" << *it << "' in namespace " << ns << std::endl;
      }
    }

    clock_gettime(CLOCK_REALTIME, &time_stop);
    milliseconds_stop = time_stop.tv_sec*1000LL + time_stop.tv_nsec/1000;
    milliseconds_delta = milliseconds_stop - milliseconds_start;
    std::cout << "\tsize=" << size << "\tt=" << (milliseconds_delta / 1000.0) << "s\t" << (static_cast<double>(urls.size()) / milliseconds_delta * 1000.0) << " articles/s" << std::endl;

    // reopen file
    file = zim::File();
    file = zim::File(filename);

    // random access
    std::cout << "random:" << std::flush;

    clock_gettime(CLOCK_REALTIME, &time_start);
    milliseconds_start = time_start.tv_sec*1000LL + time_start.tv_nsec/1000;

    size = 0;
    for (unsigned r = 0; r < randomCount; ++r)
      size += file.getArticle(ns, randomUrls[rand() % randomUrls.size()]).getData().size();

    //for (UrlsType::const_iterator it = randomUrls.begin(); it != randomUrls.end(); ++it)
      //size += file.getArticle(ns, *it).getData().size();

    clock_gettime(CLOCK_REALTIME, &time_stop);
    milliseconds_stop = time_stop.tv_sec*1000LL + time_stop.tv_nsec/1000;
    milliseconds_delta = milliseconds_stop - milliseconds_start;
    std::cout << "\tsize=" << size << "\tt=" << (milliseconds_delta / 1000.0) << "s\t" << (static_cast<double>(randomCount) / milliseconds_delta * 1000.0) << " articles/s" << std::endl;
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

