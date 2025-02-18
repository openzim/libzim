/*
 * Copyright (C) 2017-2021 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2021 Maneesh P M <manu.pm55@gmail.com>
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
#include "archive.h"
#include <vector>
#include <string>

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
 * A Searcher is a object fulltext searching a set of Archives
 *
 * A Searcher is mainly used to create new `Search`
 * Internaly, this is mainly a wrapper around a Xapian database.
 *
 * All search (at exception of SearchIterator) operation are thread safe.
 * You can freely create several Search from one Searcher and use them in different threads.
 */
class LIBZIM_API Searcher
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
    Searcher& addArchive(const Archive& archive);

    /** Create a search for a specific query.
     *
     * The search is made on all archives added to the Searcher.
     *
     * @param query The Query to search.
     *
     * @throws std::runtime_error if the searcher does not have a valid
     *         FT database.
     */
    Search search(const Query& query);

    /** Set the verbosity of search operations.
     *
     * @param verbose The verbose mode to set
     */
    void setVerbose(bool verbose);

  private: // methods
    void initDatabase();

  private: // data
    std::shared_ptr<InternalDataBase> mp_internalDb;
    std::vector<Archive> m_archives;
    bool m_verbose;
};

/**
 * A Query represent a query.
 *
 * It describe what have to be searched and how.
 * A Query is "database" independent.
 */
class LIBZIM_API Query
{
  public:
    /** Query constructor.
     *
     * Create a empty query.
     */
    Query(const std::string& query = "");

    /** Set the textual query of the Query.
     *
     * @param query The string to search for.
     */
    Query& setQuery(const std::string& query);

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

    std::string m_query { "" };

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
class LIBZIM_API Search
{
    public:
        Search(Search&& s);
        Search& operator=(Search&& s);
        ~Search();

        /** Get a set of results for this search.
         *
         * @param start The begining of the range to get
         *              (offset of the first result).
         * @param maxResults The maximum number of results to return
         *                   (offset of last result from the start of range).
         */
        const SearchResultSet getResults(int start, int maxResults) const;

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
class LIBZIM_API SearchResultSet
{
  public:
    typedef SearchIterator iterator;

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
