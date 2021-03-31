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
class InternalDataBase;

/**
 * A Searcher is a object searching a set of Archives
 *
 * A Searcher is mainly used to create new `Search`
 * Internaly, this is mainly a wrapper around a Xapian database.
 */
class Searcher
{
  public:
    explicit Searcher(const std::vector<Archive>& archives);
    explicit Searcher(const Archive& archive);
    Searcher(const Searcher& other);
    Searcher& operator=(const Searcher& other);
    Searcher(Searcher&& other);
    Searcher& operator=(Searcher&& other);
    ~Searcher();

    Searcher& add_archive(const Archive& archive);

    Search search(bool suggestionMode);

  private: // methods
    void initDatabase(bool suggestionMode);

  private: // data
    std::shared_ptr<InternalDataBase> mp_internalDb;
    std::shared_ptr<InternalDataBase> mp_internalSuggestionDb;
    std::vector<Archive> m_archives;
};


class Search
{
    friend class search_iterator;
    friend struct search_iterator::InternalData;
    public:
        typedef search_iterator iterator;

        void set_verbose(bool verbose);

        Search& set_query(const std::string& query);
        Search& set_georange(float latitude, float longitude, float distance);
        Search& set_range(int start, int end);

        search_iterator begin() const;
        search_iterator end() const;
        int get_matches_estimated() const;

    private: // methods
        Search(std::shared_ptr<InternalDataBase> p_internalDb, bool suggestionMode);

    private: // data
         struct InternalData;
         std::shared_ptr<InternalDataBase> mp_internalDb;
         mutable std::shared_ptr<InternalData> internal;

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

  friend class Searcher;
};

} //namespace zim

#endif // ZIM_SEARCH_H
