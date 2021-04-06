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

namespace Xapian {
  class Enquire;
  class MSet;
};

namespace zim
{

class Archive;
class InternalDataBase;
class Search;
class SearchResultSet;


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


/**
 * A Search represent a particular search, based on a `Searcher`.
 *
 * This is mainly a wrapper around a Xapian::Enquire.
 * The `Search` must be configured first (with `set*` methods) and
 * must be started (with `start` method)
 *
 * Results can be queried with the `get*` methods **after** the search is started.
 */
class Search
{
    public:
        Search(Search&& s);
        Search& operator=(Search&& s);
        ~Search();

        Search& setVerbose(bool verbose);
        Search& setQuery(const std::string& query);
        Search& setGeorange(float latitude, float longitude, float distance);

        const SearchResultSet getResults(int start, int end) const;
        int getEstimatedMatches() const;

    private: // methods
        Search(std::shared_ptr<InternalDataBase> p_internalDb, bool suggestionMode);
        Xapian::Enquire& getEnquire() const;

    private: // data
         std::shared_ptr<InternalDataBase> mp_internalDb;
         mutable std::unique_ptr<Xapian::Enquire> mp_enquire;

         bool m_verbose { false };
         std::string m_query { "" };
         bool m_suggestionMode { false };

         bool m_geoquery { false };
         float m_latitude { 0 };
         float m_longitude { 0 };
         float m_distance { 0 } ;

  friend class Searcher;
};

/**
 * The `SearchResult` represent a range of results corresponding to a `Search`.
 *
 * It mainly allows to get a iterator.
 */
class SearchResultSet
{
  public:
    typedef search_iterator iterator;
    iterator begin() const;
    iterator end() const;
    int size() const;

  private:
    SearchResultSet(std::shared_ptr<InternalDataBase> p_internalDb, Xapian::MSet&& mset);
    SearchResultSet(std::shared_ptr<InternalDataBase> p_internalDb);

  private: // data
    std::shared_ptr<InternalDataBase> mp_internalDb;
    std::shared_ptr<Xapian::MSet> mp_mset;
  friend class Search;
};

} //namespace zim

#endif // ZIM_SEARCH_H
