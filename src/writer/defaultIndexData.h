/*
 * Copyright (C) 2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#ifndef ZIM_WRITER_DEFAULTINDEXDATA_H
#define ZIM_WRITER_DEFAULTINDEXDATA_H

#include <zim/writer/item.h>
#include "xapian/myhtmlparse.h"
#include "tools.h"

namespace zim
{
  namespace writer
  {
    class DefaultIndexData : public IndexData {
      public:
        DefaultIndexData(const std::string& htmlData, const std::string& title)
          : title(title)
        {
#if defined(ENABLE_XAPIAN)
          try {
            htmlParser.parse_html(htmlData, "UTF-8", true);
          } catch(...) {}
#endif
        }

        bool hasIndexData() const {
#if defined(ENABLE_XAPIAN)
          return (htmlParser.dump.find("NOINDEX") == std::string::npos);
#else
          return false;
#endif
        }

        std::string getTitle() const {
#if defined(ENABLE_XAPIAN)
          return zim::removeAccents(title);
#else
          return "";
#endif
         }

        std::string getContent() const {
#if defined(ENABLE_XAPIAN)
          return zim::removeAccents(htmlParser.dump);
#else
          return "";
#endif
        }

        std::string getKeywords() const {
#if defined(ENABLE_XAPIAN)
          return zim::removeAccents(htmlParser.keywords);
#else
          return "";
#endif
        }

        uint32_t getWordCount() const {
#if defined(ENABLE_XAPIAN)
          return countWords(htmlParser.dump);
#else
          return 0;
#endif
        }

        std::tuple<bool, double, double> getGeoPosition() const
        {
#if defined(ENABLE_XAPIAN)
          if(htmlParser.has_geoPosition) {
            return std::make_tuple(true, htmlParser.latitude, htmlParser.longitude);
          }
#endif
          return std::make_tuple(false, 0, 0);
        }

      private:
#if defined(ENABLE_XAPIAN)
        zim::MyHtmlParser htmlParser;
#endif
        std::string title;

    };
  }
}

#endif // ZIM_WRITER_DEFAULTINDEXDATA_H
