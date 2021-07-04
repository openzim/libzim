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

#include <zim/suggestion.h>
#include "search_internal.h"
#include <iostream>

namespace zim
{

SuggestionSearcher::SuggestionSearcher(const Archive& archive) :
    mp_internalDb(nullptr),
    m_archive(archive)
{}

SuggestionSearcher::SuggestionSearcher(const SuggestionSearcher& other) = default;
SuggestionSearcher& SuggestionSearcher::operator=(const SuggestionSearcher& other) = default;
SuggestionSearcher::SuggestionSearcher(SuggestionSearcher&& other) = default;
SuggestionSearcher& SuggestionSearcher::operator=(SuggestionSearcher&& other) = default;
SuggestionSearcher::~SuggestionSearcher() = default;

SuggestionSearch SuggestionSearcher::suggest(const Query& query)
{
  if (!mp_internalDb) {
    initDatabase();
  }
  return SuggestionSearch(mp_internalDb, query);
}

void SuggestionSearcher::initDatabase()
{
    mp_internalDb = std::make_shared<InternalDataBase>(std::vector<Archive>{m_archive}, true);
}

SuggestionSearch::SuggestionSearch(std::shared_ptr<InternalDataBase> p_internalDb, const Query& query)
 : mp_internalDb(p_internalDb),
   mp_enquire(nullptr),
   m_query(query)
{
}

SuggestionSearch::SuggestionSearch(SuggestionSearch&& s) = default;
SuggestionSearch& SuggestionSearch::operator=(SuggestionSearch&& s) = default;
SuggestionSearch::~SuggestionSearch() = default;

int SuggestionSearch::getEstimatedMatches() const
{
  if (mp_internalDb->hasDatabase()) {
    try {
      auto enquire = getEnquire();
      // Force xapian to check at least one document even if we ask for an empty mset.
      // Else, the get_matches_estimated may be wrong and return 0 even if we have results.
      auto mset = enquire.get_mset(0, 0, 1);
      return mset.get_matches_estimated();
    } catch(...) {
      std::cerr << "Query Parsing failed, Switching to search without index." << std::endl;
    }
  }

  return mp_internalDb->m_archives[0].findByTitle(m_query.m_query).size();
}

const SuggestionResultSet SuggestionSearch::getResults(int start, int maxResults) const {
    if (mp_internalDb->hasDatabase())
    {
      try {
        auto enquire = getEnquire();
        auto mset = enquire.get_mset(start, maxResults);
        return SuggestionResultSet(mp_internalDb, std::move(mset));
      } catch(...) {
        std::cerr << "Query Parsing failed, Switching to search without index." << std::endl;
      }
    }
    return SuggestionResultSet(mp_internalDb->m_archives[0].findByTitle(m_query.m_query));
}

Xapian::Enquire& SuggestionSearch::getEnquire() const
{
    if ( mp_enquire ) {
        return *mp_enquire;
    }

    auto enquire = std::unique_ptr<Xapian::Enquire>(new Xapian::Enquire(mp_internalDb->m_database));

    auto query = mp_internalDb->parseQuery(m_query, true);
    if (m_query.m_verbose) {
        std::cout << "Parsed query '" << m_query.m_query << "' to " << query.get_description() << std::endl;
    }
    enquire->set_query(query);

   /*
    * In suggestion mode, we are searching over a separate title index. Default BM25 is not
    * adapted for this case. WDF factor(k1) controls the effect of within document frequency.
    * k1 = 0.001 reduces the effect of word repitition in document. In BM25, smaller documents
    * get larger weights, so normalising the length of documents is necessary using b = 1.
    * The document set is first sorted by their relevance score then by value so that suggestion
    * results are closer to search string.
    * refer https://xapian.org/docs/apidoc/html/classXapian_1_1BM25Weight.html
    */

    enquire->set_weighting_scheme(Xapian::BM25Weight(0.001,0,1,1,0.5));
    if (mp_internalDb->hasValue("title")) {
      enquire->set_sort_by_relevance_then_value(mp_internalDb->valueSlot("title"), false);
    }

    if (mp_internalDb->hasValue("targetPath")) {
      enquire->set_collapse_key(mp_internalDb->valueSlot("targetPath"));
    }

    mp_enquire = std::move(enquire);
    return *mp_enquire;
}

SuggestionResultSet::SuggestionResultSet(SearchResultSet searchResultSet) :
  mp_searchResultSet(std::unique_ptr<SearchResultSet>(new SearchResultSet(searchResultSet))),
  mp_entryRange(nullptr)
{}

SuggestionResultSet::SuggestionResultSet(EntryRange entryRange) :
  mp_searchResultSet(nullptr),
  mp_entryRange(std::unique_ptr<EntryRange>(new EntryRange(entryRange)))
{}

int SuggestionResultSet::size() const
{
  if (! mp_entryRange) {
      return mp_searchResultSet->size();
  }
  return mp_entryRange->size();
}

SuggestionResultSet::iterator SuggestionResultSet::begin() const
{
    if ( ! mp_entryRange ) {
        return iterator(mp_searchResultSet->begin());
    }
    return iterator(mp_entryRange->begin());
}

SuggestionResultSet::iterator SuggestionResultSet::end() const
{
    if ( ! mp_entryRange ) {
        return iterator(mp_searchResultSet->end());
    }
    return iterator(mp_entryRange->end());
}

} // namespace zim
