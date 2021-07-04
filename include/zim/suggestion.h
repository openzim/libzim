/*
 * Copyright (C) 2021 Maneesh P M <manu.pm55@gmail.com>
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
#include "search.h"
#include "archive.h"

namespace zim
{

class SuggestionSearcher;
class SuggestionSearch;
class SuggestionIterator;

class SuggestionSearcher
{
  public:
    explicit SuggestionSearcher(const Archive& archive);

    SuggestionSearcher(const SuggestionSearcher& other);
    SuggestionSearcher& operator=(const SuggestionSearcher& other);
    SuggestionSearcher(SuggestionSearcher&& other);
    SuggestionSearcher& operator=(SuggestionSearcher&& other);
    ~SuggestionSearcher();

    SuggestionSearch suggest(const Query& query);

  private: // methods
    void initDatabase();

  private: // data
    std::shared_ptr<InternalDataBase> mp_internalDb;
    Archive m_archive;
};

class SuggestionSearch
{
    public:
        SuggestionSearch(SuggestionSearch&& s);
        SuggestionSearch& operator=(SuggestionSearch&& s);
        ~SuggestionSearch();

        const SuggestionResultSet getResults(int start, int end) const;

        int getEstimatedMatches() const;

    private: // methods
        SuggestionSearch(std::shared_ptr<InternalDataBase> p_internalDb, const Query& query);
        Xapian::Enquire& getEnquire() const;

    private: // data
         std::shared_ptr<InternalDataBase> mp_internalDb;
         mutable std::unique_ptr<Xapian::Enquire> mp_enquire;
         Query m_query;

  friend class SuggestionSearcher;
};

class SuggestionResultSet
{
  public:
    typedef SuggestionIterator iterator;
    typedef Archive::EntryRange<EntryOrder::titleOrder> EntryRange;

    iterator begin() const;

    iterator end() const;

    int size() const;

  private: // data
    std::shared_ptr<SearchResultSet> mp_searchResultSet;
    std::shared_ptr<EntryRange> mp_entryRange;

  private:
    SuggestionResultSet(SearchResultSet searchResultSet);
    SuggestionResultSet(EntryRange entryRange);

  friend class SuggestionSearch;
};

} // namespace zim

#endif // ZIM_SUGGESTION_H
