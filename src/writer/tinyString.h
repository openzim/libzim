/*
 * Copyright (C) 2021 Matthieu Gautier <mgautier@mgautier.fr>
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

#ifndef ZIM_WRITER_TINYSTRING_H
#define ZIM_WRITER_TINYSTRING_H

#include "../zim_types.h"
#include <cstring>

namespace zim
{
  namespace writer {
    class TinyString {
      public: // functions
        TinyString() :
          m_data(nullptr),
          m_size(0)
        {}
        TinyString(const std::string& s) :
          m_data(new char[(uint16_t)s.size()]),
          m_size(s.size())
        {
          if (s.size() >= 0xFFFF) {
            throw std::runtime_error("String len is too big");
          }
          std::memcpy(m_data, s.data(), m_size);
        }
        TinyString(TinyString&& t):
          m_data(t.m_data),
          m_size(t.m_size)
        {
          t.m_data = nullptr;
          t.m_size = 0;
        };
        TinyString(const TinyString& t) :
          m_data(new char[(uint16_t)t.m_size]),
          m_size(t.m_size)
        {
          std::memcpy(m_data, t.m_data, m_size);
        }

        ~TinyString() {
          if (m_data) {
            delete[] m_data;
            m_data = nullptr;
          }
        }

        void operator=(const TinyString& ) = delete;

        operator std::string() const { return std::string(m_data, m_size); }
        bool empty() const { return m_size == 0; }
        size_t size() const { return m_size; }
        const char* const data() const { return m_data; }
        bool operator==(const TinyString& other) const {
          return (m_size == other.m_size) && (std::memcmp(m_data, other.m_data, m_size) == 0);
        }
        bool operator<(const TinyString& other) const {
          auto min_size = std::min(m_size, other.m_size);
          auto ret = std::memcmp(m_data, other.m_data, min_size);
          if (ret == 0) {
            return m_size < other.m_size;
          } else {
            return ret < 0;
          }
        }

      protected: // members
        char* m_data;
        uint16_t m_size;
    } PACKED;

    class PathTitleTinyString : public TinyString {
      public:
        PathTitleTinyString() : TinyString() {}
        PathTitleTinyString(const std::string& path, const std::string& title)
          : TinyString(PathTitleTinyString::concat(path, title))
        {}

        static std::string concat(const std::string& path, const std::string& title) {
          std::string result(path.data(), path.size()+1);
          if ( title != path ) {
            result += title;
          }
          return result;
        }

        void operator=(const PathTitleTinyString& ) = delete;

        std::string getPath() const {
          if (m_size == 0) {
            return std::string();
          }
          return std::string(m_data);
        }

      private:
        std::string_view getTitleAsStringView() const {
          if (m_size == 0) {
            return std::string_view();
          }
          auto title_start = std::strlen(m_data) + 1;
          if (title_start == m_size) {
            // return the path as a title
            return std::string_view(m_data, m_size-1);
          } else {
            return std::string_view(m_data+title_start, m_size-title_start);
          }
        }

      public:
        std::string getTitle() const {
          return std::string(getTitleAsStringView());
        }

        bool comparePath(const PathTitleTinyString& other) const {
          if ( m_size == 0 || other.m_size == 0 ) {
            return m_size < other.m_size;
          }

          return strcmp(m_data, other.m_data) < 0;
        }

        bool compareTitle(const PathTitleTinyString& other) const {
          return getTitleAsStringView() < other.getTitleAsStringView();
        }
    } PACKED;
  }
}

#endif // ZIM_WRITER_TINYSTRING_H

