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

InternalDataBase::InternalDataBase(const std::vector<Archive>& archives, bool verbose)
  : m_verbose(verbose)
{
    bool first = true;
    m_queryParser.set_database(m_database);
    m_queryParser.set_default_op(Xapian::Query::op::OP_AND);

    for(auto& archive: archives) {
        auto impl = archive.getImpl();
        FileImpl::FindxResult r;
        r = impl->findx('X', "fulltext/xapian");
        if (!r.first) {
          r = impl->findx('Z', "/fulltextIndex/xapian");
        }
        if (!r.first) {
            continue;
        }
        auto xapianEntry = Entry(impl, entry_index_type(r.second));
        auto accessInfo = xapianEntry.getItem().getDirectAccessInformation();
        if (accessInfo.second == 0) {
            continue;
        }

        Xapian::Database database;
        if (!getDbFromAccessInfo(accessInfo, database)) {
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
                    m_queryParser.set_stemming_strategy(Xapian::QueryParser::STEM_ALL);
                } catch (...) {
                    std::cout << "No stemming for language '" << languageLocale.getLanguage() << "'" << std::endl;
                }
            }
            auto stopwords = database.get_metadata("stopwords");
            if ( !stopwords.empty() ){
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

Xapian::Query InternalDataBase::parseQuery(const Query& query)
{
  Xapian::Query xquery;

  xquery = m_queryParser.parse_query(query.m_query);

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
    m_archives(archives),
    m_verbose(false)
{}

Searcher::Searcher(const Archive& archive) :
    mp_internalDb(nullptr),
    m_verbose(false)
{
    m_archives.push_back(archive);
}

Searcher::Searcher(const Searcher& other) = default;
Searcher& Searcher::operator=(const Searcher& other) = default;
Searcher::Searcher(Searcher&& other) = default;
Searcher& Searcher::operator=(Searcher&& other) = default;
Searcher::~Searcher() = default;

Searcher& Searcher::addArchive(const Archive& archive) {
    m_archives.push_back(archive);
    mp_internalDb.reset();
    return *this;
}

Search Searcher::search(const Query& query)
{
  if (!mp_internalDb) {
    initDatabase();
  }

  if (!mp_internalDb->hasDatabase()) {
    throw(std::runtime_error("Cannot create Search without FT Xapian index"));
  }

  return Search(mp_internalDb, query);
}

void Searcher::setVerbose(bool verbose)
{
  m_verbose = verbose;
}

void Searcher::initDatabase()
{
    mp_internalDb = std::make_shared<InternalDataBase>(m_archives, m_verbose);
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

Query::Query(const std::string& query) :
  m_query(query)
{}

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
      // Force xapian to check at least 10 documents even if we ask for an empty mset.
      // Else, the get_matches_estimated may be wrong and return 0 even if we have results.
      auto mset = enquire.get_mset(0, 0, 10);
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

    auto query = mp_internalDb->parseQuery(m_query);
    if (mp_internalDb->m_verbose) {
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
