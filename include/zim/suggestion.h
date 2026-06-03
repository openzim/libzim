/*
 * Copyright (C) 2021 Maneesh P M <manu.pm55@gmail.com>
 * Copyright (C) 2017-2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#ifndef ZIM_SUGGESTION_H
#define ZIM_SUGGESTION_H

#include "suggestion_iterator.h"
#include "archive.h"

#if defined(LIBZIM_WITH_XAPIAN)
namespace Xapian {
  class Enquire;
  class MSet;
};
#endif

namespace zim
{

class SuggestionSearcher;
class SuggestionSearch;
class SuggestionIterator;
class SuggestionDataBase;

/**
 * A SuggestionSearcher is a object suggesting over titles of an Archive
 *
 * A SuggestionSearcher is mainly used to create new `SuggestionSearch`
 * Internaly, this is a wrapper around a SuggestionDataBase with may or may not
 * include a Xapian index.
 *
 * You should consider that all search operations are NOT threadsafe.
 * It is up to you to protect your calls to avoid race competition.
 * However, SuggestionSearcher (and subsequent classes) do not maintain a global/
 * share state You can create several Searchers and use them in different threads.
 */
class LIBZIM_API SuggestionSearcher
{
  public:
    /** SuggestionSearcher constructor.
     *
     * Construct a SuggestionSearcher on top of an archive.
     *
     * @param archive An archive to suggest on.
     */
    explicit SuggestionSearcher(const Archive& archive);

    SuggestionSearcher(const SuggestionSearcher& other);
    SuggestionSearcher& operator=(const SuggestionSearcher& other);
    SuggestionSearcher(SuggestionSearcher&& other);
    SuggestionSearcher& operator=(SuggestionSearcher&& other);
    ~SuggestionSearcher();

    /** Create a SuggestionSearch for a specific query.
     *
     * The search is made on the archive under the SuggestionSearcher.
     *
     * @param query The SuggestionQuery to search.
     *
     * If the estimated count of title suggestions would exceed the specified
     * limit, automcompletion suggestions are returned instead.
     */
    SuggestionSearch suggest(const std::string& query);

    /** Set the verbosity of search operations.
     *
     * @param verbose The verbose mode to set
     */
    void setVerbose(bool verbose);

  private: // methods
    void initDatabase();

  private: // data
    std::shared_ptr<SuggestionDataBase> mp_internalDb;
    Archive m_archive;
    bool m_verbose;
};

/**
 * A SuggestionSearch represent a particular suggestion search, based on a `SuggestionSearcher`.
 */
class LIBZIM_API SuggestionSearch
{
    public: // types
        typedef std::vector<SuggestionItem> Results;

    public: // functions
        SuggestionSearch(SuggestionSearch&& s);
        SuggestionSearch& operator=(SuggestionSearch&& s);
        ~SuggestionSearch();

        /** Get a set of results for this search.
         *
         * Returns a subset of title suggestions for the requested range from
         * the full set of results.
         *
         * @param start The begining of the range to get
         *              (offset of the first result).
         * @param maxResults The maximum number of results to return
         *                   (offset of last result from the start of range).
         */
        const SuggestionResultSet getResults(int start, int maxResults) const;

        /** Get auto-completion suggestions for this search.
         *
         * Returns auto-completion suggestions for the word preceding the text
         * edit location. In the current implementation, the text edit location
         * is assumed to be at the end of the query string provided to the
         * SuggestionSearch::suggest() method. In the future the text edit
         * location will be indicated by a special code-point (e.g.
         * carriage-return, form-feed or soft-hyphen) in the query string.
         *
         * @param maxCount The maximum number of results to return.
         */
        Results getAutocompletionSuggestions(uint32_t maxCount) const;

        /** Get spelling correction suggestions for this search.
         *
         * Returns spelling correction suggestions for the word containing the
         * text edit location. In the current implementation, the text edit
         * location is assumed to be at the end of the query string provided to
         * the SuggestionSearch::suggest() method. In the future the text edit
         * location will be indicated by a special code-point (e.g.
         * carriage-return, form-feed or soft-hyphen) in the query string.
         *
         * @param maxCount The maximum number of results to return.
         */
        Results getSpellingSuggestions(uint32_t maxCount) const;

        /** Get the top suggestions for the set upper limit on their count
         *
         * This method returns the best list of suggestions (title suggestions,
         * auto-completion or spelling correction suggestions of the word at
         * the edit location or a sorted mix thereof) fitting within the
         * specified limit.
         *
         * @param maxCount The maximum number of results to return.
         */
        Results getSmartSuggestions(uint32_t maxCount) const;

        /** Get the estimated count of title matches for this suggestion search.
         *
         * As the name suggest, it is an estimation of the number of results.
         * As a member of the initial API, the name of this method conceals
         * the fact that only title suggestions are covered by it.
         */
        int getEstimatedMatches() const;

    private: // methods
        SuggestionSearch(std::shared_ptr<SuggestionDataBase> p_internalDb,
                         const std::string& query);

    private: // data
         std::shared_ptr<SuggestionDataBase> mp_internalDb;
         std::string m_query;

  friend class SuggestionSearcher;

#ifdef ZIM_PRIVATE
    public:
        // Close Xapian db to force range based search
        const void forceRangeSuggestion();
#endif

// Xapian based methods and data
#if defined(LIBZIM_WITH_XAPIAN)
    private: // Xapian based methods
        Xapian::Enquire& getEnquire() const;

    private: // Xapian based data
        mutable std::unique_ptr<Xapian::Enquire> mp_enquire;
#endif  // LIBZIM_WITH_XAPIAN
};

/**
 * `SuggestionResultSet` represents a range of results corresponding to a
 * `SuggestionSearch`.
 *
 * Its purpose is to provide uniform interface for iterating over the suggestion
 * results for various sources of suggestion data (Xapian title index or title
 * ordered listing in the absence of the former).
 */
class LIBZIM_API SuggestionResultSet
{
  public: // types
    typedef SuggestionIterator iterator;

  public: // functions
    /** The begin iterator on the result range. */
    iterator begin() const;

    /** The end iterator on the result range. */
    iterator end() const;

    /** The size of the SearchResult (end()-begin()) */
    int size() const;

  private: // types
    typedef Archive::EntryRange<EntryOrder::titleOrder> EntryRange;
    typedef std::shared_ptr<SuggestionDataBase> SuggestionDBPtr;

  private: // functions
    explicit SuggestionResultSet(EntryRange entryRange);

  private: // data
    SuggestionDBPtr mp_internalDb;
    std::shared_ptr<EntryRange> mp_entryRange;

  friend class SuggestionSearch;

// Xapian based methods and data
#if defined(LIBZIM_WITH_XAPIAN)

  private: // Xapian based methods
    SuggestionResultSet(SuggestionDBPtr p_internalDb, Xapian::MSet&& mset);

  private: // Xapian based data
    std::shared_ptr<Xapian::MSet> mp_mset;

#endif  // LIBZIM_WITH_XAPIAN
};

} // namespace zim

#endif // ZIM_SUGGESTION_H
