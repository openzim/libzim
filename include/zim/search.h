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
class Query;
class Search;
class SearchResultSet;


/**
 * A Searcher is a object searching a set of Archives
 *
 * A Searcher is mainly used to create new `Search`
 * Internaly, this is mainly a wrapper around a Xapian database.
 *
 * You should consider that all search operations are NOT threadsafe.
 * It is up to you to protect your calls to avoid race competition.
 * However, Searcher (and subsequent classes) do not maintain a global/share state.
 * You can create several Searchers and use them in different threads.
 */
class Searcher
{
  public:
    /** Searcher constructor.
     *
     * Construct a searcher on top of several archives (multi search).
     *
     * @param archives A list(vector) of archives to search on.
     */
    explicit Searcher(const std::vector<Archive>& archives);

    /** Searcher constructor.
     *
     * Construct a searcher on top of on archive.
     *
     * @param archive A archive to search on.
     */
    explicit Searcher(const Archive& archive);
    Searcher(const Searcher& other);
    Searcher& operator=(const Searcher& other);
    Searcher(Searcher&& other);
    Searcher& operator=(Searcher&& other);
    ~Searcher();

    /** Add a archive to the searcher.
     *
     * Adding a archive to a searcher do not invalidate already created search.
     */
    Searcher& add_archive(const Archive& archive);

    /** Create a search for a specific query.
     *
     * The search is made on all archives added to the Searcher.
     *
     * @param query The Query to search.
     */
    Search search(const Query& query);

  private: // methods
    void initDatabase(bool suggestionMode);

  private: // data
    std::shared_ptr<InternalDataBase> mp_internalDb;
    std::shared_ptr<InternalDataBase> mp_internalSuggestionDb;
    std::vector<Archive> m_archives;
};


/**
 * A Query represent a query.
 *
 * It describe what have to be searched and how.
 * A Query is "database" independent.
 */
class Query
{
  public:
    /** Query constructor.
     *
     * Create a empty query.
     */
    Query() = default;

    /** Set the verbosity of the Query.
     *
     * @param verbose If the query must be verbose or not.
     */
    Query& setVerbose(bool verbose);

    /** Set the textual query of the Query.
     *
     * @param query The string to search for.
     * @param suggestionMode If we should search on title (suggestionMode)
     *                       or on fulltext index.
     */
    Query& setQuery(const std::string& query, bool suggestionMode);

    /** Set the geographical query of the Query.
     *
     * Some article may be geo positioned.
     * You can search for articles in a certain distance of a point.
     *
     * @param latitude The latitute of the point.
     * @param longitude The longitude of the point.
     * @param distance The maximal distance from the point.
     */
    Query& setGeorange(float latitude, float longitude, float distance);

    bool m_verbose { false };
    std::string m_query { "" };
    bool m_suggestionMode { false };

    bool m_geoquery { false };
    float m_latitude { 0 };
    float m_longitude { 0 };
    float m_distance { 0 } ;
};


/**
 * A Search represent a particular search, based on a `Searcher`.
 *
 * This is somehow the reunification of a `Searcher` (what to search on)
 * and a `Query` (what to search for).
 */
class Search
{
    public:
        Search(Search&& s);
        Search& operator=(Search&& s);
        ~Search();

        /** Get a set of results for this search.
         *
         * @param start The begining of the range to get
         *              (offset of the first result).
         * @param end   The end of the range to get
         *              (offset of the result past the end of the range).
         */
        const SearchResultSet getResults(int start, int end) const;

        /** Get the number of estimated results for this search.
         *
         * As the name suggest, it is a estimation of the number of results.
         */
        int getEstimatedMatches() const;

    private: // methods
        Search(std::shared_ptr<InternalDataBase> p_internalDb, const Query& query);
        Xapian::Enquire& getEnquire() const;

    private: // data
         std::shared_ptr<InternalDataBase> mp_internalDb;
         mutable std::unique_ptr<Xapian::Enquire> mp_enquire;
         Query m_query;

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

    /** The begin iterator on the result range. */
    iterator begin() const;

    /** The end iterator on the result range. */
    iterator end() const;

    /** The size of the SearchResult (end()-begin()) */
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
