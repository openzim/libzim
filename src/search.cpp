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

#include <zim/search.h>
#include <zim/archive.h>
#include <zim/item.h>
#include "fileimpl.h"
#include "search_internal.h"
#include "fs.h"
#include "tools.h"

#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#if !defined(_WIN32)
# include <unistd.h>
#else
# include <io.h>
#endif
#include <errno.h>

#include "xapian.h"
#include <unicode/locid.h>

#include "constants.h"

#define MAX_MATCHES_TO_SORT 10000

namespace zim
{

InternalDataBase::InternalDataBase(const std::vector<Archive>& archives, bool suggestionMode)
  : m_suggestionMode(suggestionMode)
{
    bool first = true;
    bool hasNewSuggestionFormat = false;
    m_queryParser.set_database(m_database);
    m_queryParser.set_default_op(Xapian::Query::op::OP_AND);
    m_flags = Xapian::QueryParser::FLAG_DEFAULT;
    if (suggestionMode) {
        m_flags |= Xapian::QueryParser::FLAG_PARTIAL;
    }
    for(auto& archive: archives) {
        auto impl = archive.getImpl();
        FileImpl::FindxResult r;
        if (suggestionMode) {
          r = impl->findx('X', "title/xapian");
          if (r.first) {
            hasNewSuggestionFormat = true;
          }
        }
        if (!r.first) {
          r = impl->findx('X', "fulltext/xapian");
        }
        if (!r.first) {
          r = impl->findx('Z', "/fulltextIndex/xapian");
        }
        if (!r.first) {
            if (m_suggestionMode) {
              m_archives.push_back(archive);
            }
            continue;
        }
        auto xapianEntry = Entry(impl, entry_index_type(r.second));
        auto accessInfo = xapianEntry.getItem().getDirectAccessInformation();
        if (accessInfo.second == 0) {
            continue;
        }

        DEFAULTFS::FD databasefd;
        try {
            databasefd = DEFAULTFS::openFile(accessInfo.first);
        } catch (...) {
            std::cerr << "Impossible to open " << accessInfo.first << std::endl;
            std::cerr << strerror(errno) << std::endl;
            continue;
        }
        if (!databasefd.seek(offset_t(accessInfo.second))) {
            std::cerr << "Something went wrong seeking databasedb "
                      << accessInfo.first << std::endl;
            std::cerr << "dbOffest = " << accessInfo.second << std::endl;
            continue;
        }

        Xapian::Database database;
        try {
            database = Xapian::Database(databasefd.release());
        } catch( Xapian::DatabaseError& e) {
            std::cerr << "Something went wrong opening xapian database for zimfile "
                      << accessInfo.first << std::endl;
            std::cerr << "dbOffest = " << accessInfo.second << std::endl;
            std::cerr << "error = " << e.get_msg() << std::endl;
            continue;
        }

        if ( first ) {
            m_valuesmap = read_valuesmap(database.get_metadata("valuesmap"));
            auto language = database.get_metadata("language");
            if (language.empty() ) {
                // Database created before 2017/03 has no language metadata.
                // However, term were stemmed anyway and we need to stem our
                // search query the same the database was created.
                // So we need a language, let's use the one of the zim.
                // If zimfile has no language metadata, we can't do lot more here :/
                try {
                    language = archive.getMetadata("Language");
                } catch(...) {}
            }
            if (!language.empty()) {
                icu::Locale languageLocale(language.c_str());
                /* Configuring language base steemming */
                try {
                    m_stemmer = Xapian::Stem(languageLocale.getLanguage());
                    m_queryParser.set_stemmer(m_stemmer);
                    m_queryParser.set_stemming_strategy(
                    (m_suggestionMode && hasNewSuggestionFormat) ? Xapian::QueryParser::STEM_SOME : Xapian::QueryParser::STEM_ALL_Z);
                } catch (...) {
                    std::cout << "No steemming for language '" << languageLocale.getLanguage() << "'" << std::endl;
                }
            }
            auto stopwords = database.get_metadata("stopwords");
            if ( !stopwords.empty() && !suggestionMode ){
                std::string stopWord;
                std::istringstream file(stopwords);
                Xapian::SimpleStopper* stopper = new Xapian::SimpleStopper();
                while (std::getline(file, stopWord, '\n')) {
                    stopper->add(stopWord);
                }
                stopper->release();
                m_queryParser.set_stopper(stopper);
            }
        } else {
            std::map<std::string, int> valuesmap = read_valuesmap(database.get_metadata("valuesmap"));
            if (m_valuesmap != valuesmap ) {
                // [TODO] Ignore the database, raise a error ?
            }
        }
        m_xapianDatabases.push_back(database);
        m_database.add_database(database);
        m_archives.push_back(archive);
        first = false;
    }
}

bool InternalDataBase::hasDatabase() const
{
  return !m_xapianDatabases.empty();
}

bool InternalDataBase::hasValuesmap() const
{
  return !m_valuesmap.empty();
}

bool InternalDataBase::hasValue(const std::string& valueName) const
{
  return (m_valuesmap.find(valueName) != m_valuesmap.end());
}

int InternalDataBase::valueSlot(const std::string& valueName) const
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
Xapian::Query InternalDataBase::parseQuery(const Query& query, bool suggestionMode)
{
  Xapian::Query xquery;

  xquery = m_queryParser.parse_query(query.m_query, m_flags);

  if (suggestionMode && !query.m_query.empty()) {
    Xapian::QueryParser suggestionParser = m_queryParser;
    suggestionParser.set_default_op(Xapian::Query::op::OP_OR);
    suggestionParser.set_stemming_strategy(Xapian::QueryParser::STEM_NONE);
    Xapian::Query subquery_phrase = suggestionParser.parse_query(query.m_query);
    // Force the OP_PHRASE window to be equal to the number of terms.
    subquery_phrase = Xapian::Query(Xapian::Query::OP_PHRASE, subquery_phrase.get_terms_begin(), subquery_phrase.get_terms_end(), subquery_phrase.get_length());

    auto qs = ANCHOR_TERM + query.m_query;
    Xapian::Query subquery_anchored = suggestionParser.parse_query(qs);
    subquery_anchored = Xapian::Query(Xapian::Query::OP_PHRASE, subquery_anchored.get_terms_begin(), subquery_anchored.get_terms_end(), subquery_anchored.get_length());

    xquery = Xapian::Query(Xapian::Query::OP_OR, xquery, subquery_phrase);
    xquery = Xapian::Query(Xapian::Query::OP_OR, xquery, subquery_anchored);
  }

  if (query.m_geoquery && hasValue("geo.position")) {
    Xapian::GreatCircleMetric metric;
    Xapian::LatLongCoord centre(query.m_latitude, query.m_longitude);
    Xapian::LatLongDistancePostingSource ps(valueSlot("geo.position"), centre, metric, query.m_distance);
    Xapian::Query geoQuery(&ps);
    if (query.m_query.empty()) {
      xquery = geoQuery;
    } else {
      xquery = Xapian::Query(Xapian::Query::OP_FILTER, xquery, geoQuery);
    }
  }

  return xquery;
}

Searcher::Searcher(const std::vector<Archive>& archives) :
    mp_internalDb(nullptr),
    m_archives(archives)
{}

Searcher::Searcher(const Archive& archive) :
    mp_internalDb(nullptr)
{
    m_archives.push_back(archive);
}

Searcher::Searcher(const Searcher& other) = default;
Searcher& Searcher::operator=(const Searcher& other) = default;
Searcher::Searcher(Searcher&& other) = default;
Searcher& Searcher::operator=(Searcher&& other) = default;
Searcher::~Searcher() = default;

Searcher& Searcher::add_archive(const Archive& archive) {
    m_archives.push_back(archive);
    mp_internalDb.reset();
    return *this;
}

Search Searcher::search(const Query& query)
{
  if (!mp_internalDb) {
    initDatabase();
  }
  return Search(mp_internalDb, query);
}

void Searcher::initDatabase()
{
    mp_internalDb = std::make_shared<InternalDataBase>(m_archives, false);
}

Search::Search(std::shared_ptr<InternalDataBase> p_internalDb, const Query& query)
 : mp_internalDb(p_internalDb),
   mp_enquire(nullptr),
   m_query(query)
{
}

Search::Search(Search&& s) = default;
Search& Search::operator=(Search&& s) = default;
Search::~Search() = default;

Query& Query::setVerbose(bool verbose) {
    m_verbose = verbose;
    return *this;
}

Query& Query::setQuery(const std::string& query) {
    m_query = query;
    return *this;
}

Query& Query::setGeorange(float latitude, float longitude, float distance) {
    m_latitude = latitude;
    m_longitude = longitude;
    m_distance = distance;
    m_geoquery = true;
    return *this;
}

int Search::getEstimatedMatches() const
{
    try {
      auto enquire = getEnquire();
      // Force xapian to check at least one document even if we ask for an empty mset.
      // Else, the get_matches_estimated may be wrong and return 0 even if we have results.
      auto mset = enquire.get_mset(0, 0, 1);
      return mset.get_matches_estimated();
    } catch(Xapian::QueryParserError& e) {
      return 0;
    }
}

const SearchResultSet Search::getResults(int start, int maxResults) const {
    try {
      auto enquire = getEnquire();
      auto mset = enquire.get_mset(start, maxResults);
      return SearchResultSet(mp_internalDb, std::move(mset));
    } catch(Xapian::QueryParserError& e) {
      return SearchResultSet(mp_internalDb);
    }
}

Xapian::Enquire& Search::getEnquire() const
{
    if ( mp_enquire ) {
        return *mp_enquire;
    }

    auto enquire = std::unique_ptr<Xapian::Enquire>(new Xapian::Enquire(mp_internalDb->m_database));

    auto query = mp_internalDb->parseQuery(m_query, false);
    if (m_query.m_verbose) {
        std::cout << "Parsed query '" << m_query.m_query << "' to " << query.get_description() << std::endl;
    }
    enquire->set_query(query);

    mp_enquire = std::move(enquire);
    return *mp_enquire;
}


SearchResultSet::SearchResultSet(std::shared_ptr<InternalDataBase> p_internalDb, Xapian::MSet&& mset) :
  mp_internalDb(p_internalDb),
  mp_mset(std::make_shared<Xapian::MSet>(mset))
{}

SearchResultSet::SearchResultSet(std::shared_ptr<InternalDataBase> p_internalDb) :
  mp_internalDb(p_internalDb),
  mp_mset(nullptr)
{}

int SearchResultSet::size() const
{
  if (! mp_mset) {
      return 0;
  }
  return mp_mset->size();
}

SearchResultSet::iterator SearchResultSet::begin() const
{
    if ( ! mp_mset ) {
        return nullptr;
    }
    return new SearchIterator::InternalData(mp_internalDb, mp_mset, mp_mset->begin());
}

SearchResultSet::iterator SearchResultSet::end() const
{
    if ( ! mp_mset ) {
        return nullptr;
    }
    return new SearchIterator::InternalData(mp_internalDb, mp_mset, mp_mset->end());
}

} //namespace zim
