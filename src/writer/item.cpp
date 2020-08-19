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

#include <zim/writer/item.h>
#include <zim/writer/contentProvider.h>
#include "xapian/myhtmlparse.h"
#include "tools.h"

#include <sstream>

namespace zim
{
  namespace writer
  {
    class DefaultIndexData : public IndexData {
      public:
        DefaultIndexData(const std::string& htmlData, const std::string& title)
          : title(title)
        {
          try {
            htmlParser.parse_html(htmlData, "UTF-8", true);
          } catch(...) {}
        }

        bool hasIndexData() const {
          return (htmlParser.dump.find("NOINDEX") == std::string::npos);
        }

        std::string getTitle() const {
          return zim::removeAccents(title);
        }

        std::string getContent() const {
          return zim::removeAccents(htmlParser.dump);
        }

        std::string getKeywords() const {
          return zim::removeAccents(htmlParser.keywords);
        }

        uint32_t getWordCount() const {
          return countWords(htmlParser.dump);
        }

        std::tuple<bool, double, double> getGeoPosition() const
        {
          if(!htmlParser.has_geoPosition) {
            return std::make_tuple(false, 0, 0);
          }
          return std::make_tuple(true, htmlParser.latitude, htmlParser.longitude);
        }

      private:
        zim::MyHtmlParser htmlParser;
        std::string title;

    };


    std::unique_ptr<IndexData> Item::getIndexData() const
    {
      auto provider = getContentProvider();
      std::ostringstream ss;
      while (true) {
        auto blob = provider->feed();
        if(blob.size() == 0) {
          break;
        }
        ss << blob;
      }
      return std::unique_ptr<IndexData>(new DefaultIndexData(ss.str(), getTitle()));
    }

    Item::Hints Item::getHints() const {
      return Hints();
    }

    std::unique_ptr<ContentProvider> StringItem::getContentProvider() const
    {
      auto shared_string = std::shared_ptr<const std::string>(shared_from_this(), &content);
      return std::unique_ptr<ContentProvider>(new SharedStringProvider(shared_string));
    }

    std::unique_ptr<IndexData> StringItem::getIndexData() const
    {
      return std::unique_ptr<IndexData>(new DefaultIndexData(content, title));
    }


    std::unique_ptr<ContentProvider> FileItem::getContentProvider() const
    {
      return std::unique_ptr<ContentProvider>(new FileProvider(filepath));
    }


  }
}
