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

#define ZIM_PRIVATE

#include <zim/suggestion.h>
#include <zim/item.h>
#include "suggestion_internal.h"
#include "fileimpl.h"
#include "tools.h"
#include "constants.h"

#if defined(ENABLE_XAPIAN)
#include <unicode/locid.h>
#endif  // ENABLE_XAPIAN

namespace zim
{

SuggestionDataBase::SuggestionDataBase(const Archive& archive, bool verbose)
  : m_archive(archive),
    m_verbose(verbose)
{
// Initialize Xapian DB if it is enabled
#if defined(ENABLE_XAPIAN)
  initXapianDb();
#endif  // ENABLE_XAPIAN
}

#if defined(ENABLE_XAPIAN)
void SuggestionDataBase::initXapianDb() {
  m_queryParser.set_database(m_database);
  m_queryParser.set_default_op(Xapian::Query::op::OP_AND);

  auto impl = m_archive.getImpl();
  FileImpl::FindxResult r;

  r = impl->findx('X', "title/xapian");
  if (!r.first) {
    return;
  }

  auto xapianEntry = Entry(impl, entry_index_type(r.second));
  auto accessInfo = xapianEntry.getItem().getDirectAccessInformation();
  if (accessInfo.second == 0) {
      return;
  }

  Xapian::Database database;
  if (!getDbFromAccessInfo(accessInfo, database)) {
    return;
  }

  m_valuesmap = read_valuesmap(database.get_metadata("valuesmap"));
  auto language = database.get_metadata("language");
  if (language.empty() ) {
      // Database created before 2017/03 has no language metadata.
      // However, term were stemmed anyway and we need to stem our
      // search query the same the database was created.
      // So we need a language, let's use the one of the zim.
      // If zimfile has no language metadata, we can't do lot more here :/
      try {
          language = m_archive.getMetadata("Language");
      } catch(...) {}
  }
  if (!language.empty()) {
      icu::Locale languageLocale(language.c_str());
      /* Configuring language base steemming */
      try {
          m_stemmer = Xapian::Stem(languageLocale.getLanguage());
          m_queryParser.set_stemmer(m_stemmer);
      } catch (...) {
          std::cout << "No stemming for language '" << languageLocale.getLanguage() << "'" << std::endl;
      }
  }

  m_database = database;
}

bool SuggestionDataBase::hasDatabase() const
{
  return !m_database.internal.empty();
}

bool SuggestionDataBase::hasValuesmap() const
{
  return !m_valuesmap.empty();
}

bool SuggestionDataBase::hasValue(const std::string& valueName) const
{
  return (m_valuesmap.find(valueName) != m_valuesmap.end());
}

int SuggestionDataBase::valueSlot(const std::string& valueName) const
{
  return m_valuesmap.at(valueName);
}

/*
 * subquery_phrase: selects documents that have the terms in the order of the query
 * within a specified window.
 * subquery_anchored: selects documents that have the terms in the order of the
 * query within a specified window and starts from the beginning of the document.
 * subquery_and: selects documents that have all the terms in the query.
 *
 * subquery_phrase and subquery_anchored by themselves are quite exclusive. To
 * include more "similar" docs, we combine them with subquery_and using OP_OR
 * operator. If a particular document has a weight of A in subquery_and and B
 * in subquery_phrase and C in subquery_anchored, the net weight of that document
 * becomes A+B+C (normalised out of 100). So the documents closer to the query
 * gets a higher relevance.
 */
Xapian::Query SuggestionDataBase::parseQuery(const std::string& query)
{
  std::lock_guard<std::mutex> locker(m_mutex);
  Xapian::Query xquery;

  const auto flags = Xapian::QueryParser::FLAG_DEFAULT | Xapian::QueryParser::FLAG_PARTIAL | Xapian::QueryParser::FLAG_CJK_NGRAM;

  // Reset stemming strategy for normal parsing
  m_queryParser.set_stemming_strategy(Xapian::QueryParser::STEM_SOME);
  xquery = m_queryParser.parse_query(query, flags);

  if ( !query.empty() && xquery.get_num_subqueries() == 0 ) {
    // a non-empty query string produced an empty xapian query which means
    // that the query string is made solely of punctuation.
    xquery = Xapian::Query(Xapian::Query::OP_WILDCARD, query);
  } else if (!query.empty()) {
    // Reconfigure stemming strategy for phrase search
    m_queryParser.set_stemming_strategy(Xapian::QueryParser::STEM_NONE);

    Xapian::Query subquery_phrase = m_queryParser.parse_query(query, Xapian::QueryParser::FLAG_CJK_NGRAM);
    // Force the OP_PHRASE window to be equal to the number of terms.
    subquery_phrase = Xapian::Query(Xapian::Query::OP_PHRASE, subquery_phrase.get_terms_begin(), subquery_phrase.get_terms_end(), subquery_phrase.get_length());

    auto qs = ANCHOR_TERM + query;
    Xapian::Query subquery_anchored = m_queryParser.parse_query(qs, Xapian::QueryParser::FLAG_CJK_NGRAM);
    subquery_anchored = Xapian::Query(Xapian::Query::OP_PHRASE, subquery_anchored.get_terms_begin(), subquery_anchored.get_terms_end(), subquery_anchored.get_length());

    xquery = Xapian::Query(Xapian::Query::OP_OR, xquery, subquery_phrase);
    xquery = Xapian::Query(Xapian::Query::OP_OR, xquery, subquery_anchored);
  }

  return xquery;
}

#endif  // ENABLE_XAPIAN

SuggestionSearcher::SuggestionSearcher(const Archive& archive) :
    mp_internalDb(nullptr),
    m_archive(archive),
    m_verbose(false)
{}

SuggestionSearcher::SuggestionSearcher(const SuggestionSearcher& other) = default;
SuggestionSearcher& SuggestionSearcher::operator=(const SuggestionSearcher& other) = default;
SuggestionSearcher::SuggestionSearcher(SuggestionSearcher&& other) = default;
SuggestionSearcher& SuggestionSearcher::operator=(SuggestionSearcher&& other) = default;
SuggestionSearcher::~SuggestionSearcher() = default;

SuggestionSearch SuggestionSearcher::suggest(const std::string& query)
{
  if (!mp_internalDb) {
    initDatabase();
  }
  return SuggestionSearch(mp_internalDb, query);
}

void SuggestionSearcher::setVerbose(bool verbose)
{
  m_verbose = verbose;
}

void SuggestionSearcher::initDatabase()
{
    mp_internalDb = std::make_shared<SuggestionDataBase>(m_archive, m_verbose);
}

SuggestionSearch::SuggestionSearch(std::shared_ptr<SuggestionDataBase> p_internalDb, const std::string& query)
 : mp_internalDb(p_internalDb),
   m_query(query)
#if defined(ENABLE_XAPIAN)
   , mp_enquire(nullptr)
#endif  // ENABLE_XAPIAN
{}

SuggestionSearch::SuggestionSearch(SuggestionSearch&& s) = default;
SuggestionSearch& SuggestionSearch::operator=(SuggestionSearch&& s) = default;
SuggestionSearch::~SuggestionSearch() = default;

int SuggestionSearch::getEstimatedMatches() const
{
#if defined(ENABLE_XAPIAN)
  if (mp_internalDb->hasDatabase()) {
    try {
      auto enquire = getEnquire();
      // Force xapian to check at least 10 documents even if we ask for an empty mset.
      // Else, the get_matches_estimated may be wrong and return 0 even if we have results.
      auto mset = enquire.get_mset(0, 0, 10);
      return mset.get_matches_estimated();
    } catch(...) {
      std::cerr << "Query Parsing failed, Switching to search without index." << std::endl;
    }
  }
#endif  // ENABLE_XAPIAN

  return mp_internalDb->m_archive.findByTitle(m_query).size();
}

const SuggestionResultSet SuggestionSearch::getResults(int start, int maxResults) const {
#if defined(ENABLE_XAPIAN)
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
#endif  // ENABLE_XAPIAN

    auto entryRange = mp_internalDb->m_archive.findByTitle(m_query);
    entryRange.offset(start, maxResults);
    return SuggestionResultSet(entryRange);
}

const void SuggestionSearch::forceRangeSuggestion() {
#if defined(ENABLE_XAPIAN)
  mp_internalDb->m_database.close();
#endif  // ENABLE_XAPIAN
}

#if defined(ENABLE_XAPIAN)
Xapian::Enquire& SuggestionSearch::getEnquire() const
{
    if ( mp_enquire ) {
        return *mp_enquire;
    }

    auto enquire = std::unique_ptr<Xapian::Enquire>(new Xapian::Enquire(mp_internalDb->m_database));

    const auto unaccentedQuery = removeAccents(m_query);
    auto query = mp_internalDb->parseQuery(unaccentedQuery);
    if (mp_internalDb->m_verbose) {
        std::cout << "Parsed query '" << unaccentedQuery << "' to " << query.get_description() << std::endl;
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

SuggestionResultSet::SuggestionResultSet(std::shared_ptr<SuggestionDataBase> p_internalDb, Xapian::MSet&& mset) :
  mp_internalDb(p_internalDb),
  mp_entryRange(nullptr),
  mp_mset(std::make_shared<Xapian::MSet>(mset))
{}
#endif  // ENABLE_XAPIAN

SuggestionResultSet::SuggestionResultSet(EntryRange entryRange) :
  mp_internalDb(nullptr),
  mp_entryRange(std::unique_ptr<EntryRange>(new EntryRange(entryRange)))
#if defined(ENABLE_XAPIAN)
  , mp_mset(nullptr)
#endif  // ENABLE_XAPIAN
{}

int SuggestionResultSet::size() const
{
#if defined(ENABLE_XAPIAN)
  if (! mp_entryRange) {
      return mp_mset->size();
  }
#endif  // ENABLE_XAPIAN

  return mp_entryRange->size();
}

SuggestionResultSet::iterator SuggestionResultSet::begin() const
{
#if defined(ENABLE_XAPIAN)
    if ( ! mp_entryRange ) {
        return new iterator::SuggestionInternalData(mp_internalDb, mp_mset, mp_mset->begin());
    }
#endif  // ENABLE_XAPIAN

    return iterator(mp_entryRange->begin());
}

SuggestionResultSet::iterator SuggestionResultSet::end() const
{
#if defined(ENABLE_XAPIAN)
    if ( ! mp_entryRange ) {
        return new iterator::SuggestionInternalData(mp_internalDb, mp_mset, mp_mset->end());
    }
#endif  // ENABLE_XAPIAN

    return iterator(mp_entryRange->end());
}

} // namespace zim
