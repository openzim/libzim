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

#ifndef ZIM_SEARCH_H
#define ZIM_SEARCH_H

#include "search_iterator.h"
#include <vector>
#include <string>
#include <map>

namespace zim
{

class Archive;
class Search
{
    friend class search_iterator;
    friend struct search_iterator::InternalData;
    public:
        typedef search_iterator iterator;

        explicit Search(const std::vector<Archive>& archives);
        explicit Search(const Archive& archive);
        Search(const Search& it);
        Search& operator=(const Search& it);
        Search(Search&& it);
        Search& operator=(Search&& it);
        ~Search();

        void set_verbose(bool verbose);

        Search& add_archive(const Archive& archive);
        Search& set_query(const std::string& query);
        Search& set_georange(float latitude, float longitude, float distance);
        Search& set_range(int start, int end);
        Search& set_suggestion_mode(bool suggestion_mode);

        search_iterator begin() const;
        search_iterator end() const;
        int get_matches_estimated() const;

    private: // methods
        void initDatabase() const;

    private: // data
         struct InternalData;
         mutable std::shared_ptr<InternalData> internal;
         std::vector<Archive> m_archives;

         std::string query;
         float latitude;
         float longitude;
         float distance;
         int range_start;
         int range_end;
         bool suggestion_mode;
         bool geo_query;
         mutable bool search_started;
         mutable bool verbose;
         mutable int estimated_matches_number;
};

} //namespace zim

#endif // ZIM_SEARCH_H
