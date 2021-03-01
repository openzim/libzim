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

#include <atomic>
#include <mutex>
#include <sstream>

namespace zim
{
  namespace writer
  {
    class DefaultIndexData : public IndexData {
      public:
        DefaultIndexData(std::unique_ptr<ContentProvider> contentProvider, const std::string& title)
          : m_initialized(false),
            mp_contentProvider(std::move(contentProvider)),
#if defined(ENABLE_XAPIAN)
            m_title(zim::removeAccents(title)),
#else
            m_title(""),
#endif
            m_hasIndexData(false),
            m_content(""),
            m_keywords(""),
            m_wordCount(0),
            m_geoPosition(std::make_tuple(false, 0, 0))
        {}

        void initialize() const {
          if (m_initialized) {
            return;
          }
          std::lock_guard<std::mutex> lock(m_initLock);
          // We have to do a double check to be sure that two call on a un-initialized object
          // will not be initiialized twice.
          if (m_initialized) {
            return;
          }
#if defined(ENABLE_XAPIAN)
          try {
            std::ostringstream ss;
            while (true) {
              auto blob = mp_contentProvider->feed();
              if(blob.size() == 0) {
                break;
              }
              ss << blob;
            }
            MyHtmlParser htmlParser;
            htmlParser.parse_html(ss.str(), "UTF-8", true);
            m_hasIndexData = (htmlParser.dump.find("NOINDEX") == std::string::npos);
            m_content = zim::removeAccents(htmlParser.dump);
            m_keywords = zim::removeAccents(htmlParser.keywords);
            m_wordCount = countWords(htmlParser.dump);
            if(htmlParser.has_geoPosition) {
              m_geoPosition = std::make_tuple(true, htmlParser.latitude, htmlParser.longitude);
            }
          } catch(...) {}
#endif
          m_initialized = true;
        }

        bool hasIndexData() const {
          initialize();
          return m_hasIndexData;
        }

        std::string getTitle() const {
          return m_title;
         }

        std::string getContent() const {
          initialize();
          return m_content;
        }

        std::string getKeywords() const {
          initialize();
          return m_keywords;
        }

        uint32_t getWordCount() const {
          initialize();
          return m_wordCount;
        }

        GeoPosition getGeoPosition() const
        {
          initialize();
          return m_geoPosition;
        }

      private:
        mutable std::atomic<bool> m_initialized;
        mutable std::mutex m_initLock;
        std::unique_ptr<ContentProvider> mp_contentProvider;
        std::string m_title;
        mutable bool m_hasIndexData;
        mutable std::string m_content;
        mutable std::string m_keywords;
        mutable uint32_t m_wordCount;
        mutable GeoPosition m_geoPosition;
    };
  }
}

#endif // ZIM_WRITER_DEFAULTINDEXDATA_H
