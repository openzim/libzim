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

namespace {
/* Split string in a token array */
std::vector<std::string> split(const std::string & str,
                                const std::string & delims=" *-")
{
  std::string::size_type lastPos = str.find_first_not_of(delims, 0);
  std::string::size_type pos = str.find_first_of(delims, lastPos);
  std::vector<std::string> tokens;

  while (std::string::npos != pos || std::string::npos != lastPos)
    {
      tokens.push_back(str.substr(lastPos, pos - lastPos));
      lastPos = str.find_first_not_of(delims, pos);
      pos     = str.find_first_of(delims, lastPos);
    }

  return tokens;
}

std::map<std::string, int> read_valuesmap(const std::string &s) {
    std::map<std::string, int> result;
    std::vector<std::string> elems = split(s, ";");
    for(std::vector<std::string>::iterator elem = elems.begin();
        elem != elems.end();
        elem++)
    {
        std::vector<std::string> tmp_elems = split(*elem, ":");
        result.insert( std::pair<std::string, int>(tmp_elems[0], atoi(tmp_elems[1].c_str())) );
    }
    return result;
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
Xapian::Query parse_query(Xapian::QueryParser* query_parser, std::string qs, int flags, std::string prefix, bool suggestion_mode) {
    Xapian::Query query, subquery_and;
    query = subquery_and = query_parser->parse_query(qs, flags, prefix);

    if (suggestion_mode && !query.empty()) {
      Xapian::Query subquery_phrase, subquery_anchored;
      query_parser->set_default_op(Xapian::Query::op::OP_OR);
      query_parser->set_stemming_strategy(Xapian::QueryParser::STEM_NONE);

      subquery_phrase = query_parser->parse_query(qs);
      subquery_phrase = Xapian::Query(Xapian::Query::OP_PHRASE, subquery_phrase.get_terms_begin(), subquery_phrase.get_terms_end(), subquery_phrase.get_length());

      qs = ANCHOR_TERM + qs;
      subquery_anchored = query_parser->parse_query(qs);
      subquery_anchored = Xapian::Query(Xapian::Query::OP_PHRASE, subquery_anchored.get_terms_begin(), subquery_anchored.get_terms_end(), subquery_anchored.get_length());

      query = Xapian::Query(Xapian::Query::OP_OR, query, subquery_phrase);
      query = Xapian::Query(Xapian::Query::OP_OR, query, subquery_anchored);
    }

    return query;
}

} // end of anonymous namespace


InternalDataBase::InternalDataBase(const std::vector<Archive>& archives, bool suggestionMode)
  : m_suggestionMode(suggestionMode),
    m_hasNewSuggestionFormat(false)
{
    bool first = true;
    for(auto& archive: archives) {
        auto impl = archive.getImpl();
        FileImpl::FindxResult r;
        if (suggestionMode) {
          r = impl->findx('X', "title/xapian");
          if (r.first) {
            m_hasNewSuggestionFormat = true;
          }
        }
        if (!r.first) {
          r = impl->findx('X', "fulltext/xapian");
        }
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
            m_language = database.get_metadata("language");
            if (m_language.empty() ) {
                // Database created before 2017/03 has no language metadata.
                // However, term were stemmed anyway and we need to stem our
                // search query the same the database was created.
                // So we need a language, let's use the one of the zim.
                // If zimfile has no language metadata, we can't do lot more here :/
                try {
                    m_language = archive.getMetadata("Language");
                } catch(...) {}
            }
            m_stopwords = database.get_metadata("stopwords");
            m_prefixes = database.get_metadata("prefixes");
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

void InternalDataBase::setupQueryparser(
  Xapian::QueryParser* queryParser,
  bool suggestionMode)
{
    queryParser->set_default_op(Xapian::Query::op::OP_AND);
    queryParser->set_database(m_database);
    if ( ! m_language.empty() ) {
        /* Build ICU Local object to retrieve ISO-639 language code (from
           ISO-639-3) */
        icu::Locale languageLocale(m_language.c_str());

        /* Configuring language base steemming */
        try {
            Xapian::Stem stemmer = Xapian::Stem(languageLocale.getLanguage());
            queryParser->set_stemmer(stemmer);
            queryParser->set_stemming_strategy(
            m_hasNewSuggestionFormat ? Xapian::QueryParser::STEM_SOME : Xapian::QueryParser::STEM_ALL);
        } catch (...) {
            std::cout << "No steemming for language '" << languageLocale.getLanguage() << "'" << std::endl;
        }
    }

    if ( !m_stopwords.empty() && !suggestionMode ){
        std::string stopWord;
        std::istringstream file(m_stopwords);
        Xapian::SimpleStopper* stopper = new Xapian::SimpleStopper();
        while (std::getline(file, stopWord, '\n')) {
            stopper->add(stopWord);
        }
        stopper->release();
        queryParser->set_stopper(stopper);
    }
}

Searcher::Searcher(const std::vector<Archive>& archives) :
    mp_internalDb(nullptr),
    mp_internalSuggestionDb(nullptr),
    m_archives(archives)
{}

Searcher::Searcher(const Archive& archive) :
    mp_internalDb(nullptr),
    mp_internalSuggestionDb(nullptr)
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
    mp_internalSuggestionDb.reset();
    return *this;
}

Search Searcher::search(const Query& query)
{
  if (query.m_suggestionMode) {
    if (!mp_internalSuggestionDb) {
      initDatabase(true);
    }
    return Search(mp_internalSuggestionDb, query);
  } else {
    if (!mp_internalDb) {
      initDatabase(false);
    }
    return Search(mp_internalDb, query);
  }
}

void Searcher::initDatabase(bool suggestionMode)
{
  if (suggestionMode) {
    mp_internalSuggestionDb = std::make_shared<InternalDataBase>(m_archives, true);
  } else {
    mp_internalDb = std::make_shared<InternalDataBase>(m_archives, false);
  }
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

Query& Query::setQuery(const std::string& query, bool suggestionMode) {
    m_query = query;
    m_suggestionMode = suggestionMode;
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

const SearchResultSet Search::getResults(int start, int end) const {
    try {
      auto enquire = getEnquire();
      auto mset = enquire.get_mset(start, end);
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

    Xapian::QueryParser* queryParser = new Xapian::QueryParser();
    if (m_query.m_verbose) {
      std::cout << "Setup queryparser using language " << mp_internalDb->m_language << std::endl;
    }
    mp_internalDb->setupQueryparser(queryParser, m_query.m_suggestionMode);

    std::string prefix = "";
    unsigned flags = Xapian::QueryParser::FLAG_DEFAULT;
    if (m_query.m_suggestionMode) {
      if (m_query.m_verbose) {
        std::cout << "Mark query as 'partial'" << std::endl;
      }
      flags |= Xapian::QueryParser::FLAG_PARTIAL;
      if ( !mp_internalDb->m_hasNewSuggestionFormat
        && mp_internalDb->m_prefixes.find("S") != std::string::npos ) {
        if (m_query.m_verbose) {
          std::cout << "Searching in title namespace" << std::endl;
        }
        prefix = "S";
      }
    }
    auto query = parse_query(queryParser, m_query.m_query, flags, prefix, m_query.m_suggestionMode);
    if (m_query.m_verbose) {
        std::cout << "Parsed query '" << m_query.m_query << "' to " << query.get_description() << std::endl;
    }
    delete queryParser;

    auto enquire = std::unique_ptr<Xapian::Enquire>(new Xapian::Enquire(mp_internalDb->m_database));

    if (m_query.m_geoquery && mp_internalDb->hasValue("geo.position")) {
        Xapian::GreatCircleMetric metric;
        Xapian::LatLongCoord centre(m_query.m_latitude, m_query.m_longitude);
        Xapian::LatLongDistancePostingSource ps(mp_internalDb->valueSlot("geo.position"), centre, metric, m_query.m_distance);
        if ( m_query.m_query.empty()) {
          query = Xapian::Query(&ps);
        } else {
          query = Xapian::Query(Xapian::Query::OP_FILTER, query, Xapian::Query(&ps));
        }
    }

   /*
    * In suggestion mode, we are searching over a separate title index. Default BM25 is not
    * adapted for this case. WDF factor(k1) controls the effect of within document frequency.
    * k1 = 0.001 reduces the effect of word repitition in document. In BM25, smaller documents
    * get larger weights, so normalising the length of documents is necessary using b = 1.
    * The document set is first sorted by their relevance score then by value so that suggestion
    * results are closer to search string.
    * refer https://xapian.org/docs/apidoc/html/classXapian_1_1BM25Weight.html
    */
    if (m_query.m_suggestionMode) {
      enquire->set_weighting_scheme(Xapian::BM25Weight(0.001,0,1,1,0.5));
      if (mp_internalDb->hasValue("title")) {
        enquire->set_sort_by_relevance_then_value(mp_internalDb->valueSlot("title"), false);
      }
    }

    enquire->set_query(query);


    if (m_query.m_suggestionMode && mp_internalDb->hasValue("targetPath")) {
      enquire->set_collapse_key(mp_internalDb->valueSlot("targetPath"));
    }

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
    return new search_iterator::InternalData(mp_internalDb, mp_mset, mp_mset->begin());
}

SearchResultSet::iterator SearchResultSet::end() const
{
    if ( ! mp_mset ) {
        return nullptr;
    }
    return new search_iterator::InternalData(mp_internalDb, mp_mset, mp_mset->end());
}

} //namespace zim
