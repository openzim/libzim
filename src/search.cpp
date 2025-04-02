/*
 * Copyright (C) 2017-2021 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2021 Maneesh P M <manu.pm55@gmail.com>
 * Copyright (C) 2021 Veloman Yunkan
 * Copyright (C) 2020 Emmanuel Engelhart <kelson@kiwix.org>
 * Copyright (C) 2018 Kunal Mehta <legoktm@member.fsf.org>
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

#include <zim/error.h>
#include <zim/search.h>
#include <zim/archive.h>
#include <zim/item.h>
#include "fileimpl.h"
#include "search_internal.h"
#include "tools.h"
#include "zim/zim.h"

#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#if !defined(_WIN32)
# include <unistd.h>
#else
# include <io.h>
#endif

#include "xapian.h"
#include <unicode/locid.h>

#include "constants.h"

#define MAX_MATCHES_TO_SORT 10000

namespace zim
{
XapianDbMetadata::XapianDbMetadata(const Xapian::Database& db, std::string defaultLanguage)
    : m_language(defaultLanguage)
{
    m_valuesmap = read_valuesmap(db.get_metadata("valuesmap"));
    auto language = db.get_metadata("language");
    if (! language.empty()) {
        m_language = language;
    }
    if (!m_language.empty()) {
        icu::Locale languageLocale(language.c_str());
        /* Configuring language base steemming */
        try {
            m_stemmer = Xapian::Stem(languageLocale.getLanguage());
        } catch (...) {
            std::cout << "No stemming for language '" << languageLocale.getLanguage() << "'" << std::endl;
        }
    }
    m_stopwords = db.get_metadata("stopwords");
}


Xapian::Stopper* XapianDbMetadata::new_stopper() {
    // Xapian (for stopper) use a internal intrusive smart pointer with a optional ref count.
    // By default (it is not ref counted) so it is to us to delete it.
    // But if we call `release` on it, it is then ref counted and pass it to Xapian to
    // let it handle the deletion. We may delete it ourselves
    // (but as any other deleted value, we must ensure no use after delete)
    if ( !m_stopwords.empty() ){
        std::string stopWord;
        std::istringstream file(m_stopwords);
        Xapian::SimpleStopper*  stopper = new Xapian::SimpleStopper();
        while (std::getline(file, stopWord, '\n')) {
            stopper->add(stopWord);
        }
        return stopper->release();
    }
    return nullptr;
}

XapianDb::XapianDb(const Xapian::Database& db, std::string defaultLanguage)
  : m_metadata(db, defaultLanguage),
    m_db(db)
{}

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
        if (!accessInfo.isValid()) {
            continue;
        }

        Xapian::Database xapianDatabase;
        if (!getDbFromAccessInfo(accessInfo, xapianDatabase)) {
          continue;
        }

        try {
            std::string defaultLanguage;
            // Database created before 2017/03 has no language metadata.
            // However, term were stemmed anyway and we need to stem our
            // search query the same the database was created.
            // So we need a language, let's use the one of the zim.
            // If zimfile has no language metadata, we can't do lot more here :/
            try {
                defaultLanguage = archive.getMetadata("Language");
            } catch(...) {}

            auto database = XapianDb(xapianDatabase, defaultLanguage);

            if ( first ) {
                m_metadata = database.m_metadata;
                m_queryParser.set_stemmer(m_metadata.m_stemmer);
                        m_queryParser.set_stemming_strategy(Xapian::QueryParser::STEM_ALL);
                m_queryParser.set_stopper(m_metadata.new_stopper());
                first = false;
            }
            m_database.add_database(database.m_db);
            m_archives.push_back(archive);
        } catch( Xapian::DatabaseError& e ) {
            // [TODO] Ignore the database or raise a error ?
            // As we already ignore the database if `getDbFromAccessInfo` "detects" a DatabaseError,
            // we also ignore here.
        }
    }
}

bool InternalDataBase::hasDatabase() const
{
  return !m_archives.empty();
}

bool InternalDataBase::hasValuesmap() const
{
  return m_metadata.hasValuesmap();
}

bool InternalDataBase::hasValue(const std::string& valueName) const
{
  return m_metadata.hasValue(valueName);
}

int InternalDataBase::valueSlot(const std::string& valueName) const
{
  return m_metadata.valueSlot(valueName);
}

Xapian::Query InternalDataBase::parseQuery(const Query& query)
{
  Xapian::Query xquery;

  const auto unaccentedQuery = removeAccents(query.m_query);
  xquery = m_queryParser.parse_query(unaccentedQuery, Xapian::QueryParser::FLAG_CJK_NGRAM);

  if (query.m_geoquery && hasValue("geo.position")) {
    Xapian::GreatCircleMetric metric;
    Xapian::LatLongCoord centre(query.m_latitude, query.m_longitude);
    Xapian::LatLongDistancePostingSource ps(valueSlot("geo.position"), centre, metric, query.m_distance);
    Xapian::Query geoQuery(&ps);
    if (unaccentedQuery.empty()) {
      xquery = geoQuery;
    } else {
      xquery = Xapian::Query(Xapian::Query::OP_FILTER, xquery, geoQuery);
    }
  }

  return xquery;
}

Searcher::Searcher(const std::vector<Archive>& archives) :
    mp_internalDb(nullptr),
    m_verbose(false)
{
    for ( const auto& a : archives ) {
        addArchive(a);
    }
}

Searcher::Searcher(const Archive& archive) :
    mp_internalDb(nullptr),
    m_verbose(false)
{
    addArchive(archive);
}

Searcher::Searcher(const Searcher& other) = default;
Searcher& Searcher::operator=(const Searcher& other) = default;
Searcher::Searcher(Searcher&& other) = default;
Searcher& Searcher::operator=(Searcher&& other) = default;
Searcher::~Searcher() = default;

namespace
{

bool archivesAreEquivalent(const Archive& a1, const Archive& a2)
{
  return a1.getUuid() == a2.getUuid();
}

bool contains(const std::vector<Archive>& archives, const Archive& newArchive)
{
    for ( const auto& a : archives ) {
        if ( archivesAreEquivalent(a, newArchive) ) {
            return true;
        }
    }
    return false;
}

} // unnamed namespace

Searcher& Searcher::addArchive(const Archive& archive) {
    if ( !contains(m_archives, archive) ) {
        m_archives.push_back(archive);
        mp_internalDb.reset();
    }
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
    } catch(Xapian::DatabaseError& e) {
      throw zim::ZimFileFormatError(e.get_description());
    }
}

const SearchResultSet Search::getResults(int start, int maxResults) const {
    try {
      auto enquire = getEnquire();
      auto mset = enquire.get_mset(start, maxResults);
      return SearchResultSet(mp_internalDb, std::move(mset));
    } catch(Xapian::QueryParserError& e) {
      return SearchResultSet(mp_internalDb);
    } catch(Xapian::DatabaseError& e) {
      throw zim::ZimFileFormatError(e.get_description());
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
  try {
      return mp_mset->size();
  } catch(Xapian::DatabaseError& e) {
    throw zim::ZimFileFormatError(e.get_description());
  }
}

SearchResultSet::iterator SearchResultSet::begin() const
{
    if ( ! mp_mset ) {
        return nullptr;
    }
    try {
        return new SearchIterator::InternalData(mp_internalDb, mp_mset, mp_mset->begin());
    } catch(Xapian::DatabaseError& e) {
        throw zim::ZimFileFormatError(e.get_description());
    }
}

SearchResultSet::iterator SearchResultSet::end() const
{
    if ( ! mp_mset ) {
        return nullptr;
    }
    try {
        return new SearchIterator::InternalData(mp_internalDb, mp_mset, mp_mset->end());
    } catch(Xapian::DatabaseError& e) {
        throw zim::ZimFileFormatError(e.get_description());
    }
}

} //namespace zim
