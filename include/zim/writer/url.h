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

#ifndef ZIM_WRITER_URL_H
#define ZIM_WRITER_URL_H

#include <string>

namespace zim
{
  namespace writer
  {
    class Url {
      public:
        Url() :
          url(),
          ns(0)
        {}
        Url(char ns, std::string url) :
          url(url),
          ns(ns)
        {}
        Url(std::string url) :
          url(url.substr(2)),
          ns(url[0])
        {}
        char getNs() const { return ns; }
        const std::string& getUrl() const { return url; }
        std::string getLongUrl() const { return std::string(1, ns) + '/' + url; }
        bool empty() const { return ns == 0 && url.empty(); }
      private:
        std::string url;
        char ns;
      friend bool operator< (const Url& lhs, const Url& rhs);
      friend bool operator== (const  Url& lhs, const Url& rhs);
    };

    inline bool operator< (const Url& lhs, const Url& rhs) {
        return lhs.ns < rhs.ns
          ||   (lhs.ns == rhs.ns && lhs.url < rhs.url);
    }
    inline bool operator== (const Url& lhs, const Url& rhs) {
        return lhs.ns == rhs.ns && lhs.url == rhs.url;
    }
  }
}

#endif // ZIM_WRITER_URL_H
