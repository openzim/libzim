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

Search Searcher::search(bool suggestionMode)
{
  if (suggestionMode) {
    if (!mp_internalSuggestionDb) {
      initDatabase(true);
    }
    return Search(mp_internalSuggestionDb, true);
  } else {
    if (!mp_internalDb) {
      initDatabase(false);
    }
    return Search(mp_internalDb, false);
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

Search::Search(std::shared_ptr<InternalDataBase> p_internalDb, bool suggestionMode)
 : mp_internalDb(p_internalDb),
   internal(std::make_shared<InternalData>()),
   query(""),
   latitude(0), longitude(0), distance(0),
   range_start(0), range_end(0),
   suggestion_mode(suggestionMode),
   geo_query(false),
   search_started(false),
   verbose(false),
   estimated_matches_number(0)
{
}


void Search::set_verbose(bool verbose) {
    this->verbose = verbose;
}

Search& Search::set_query(const std::string& query) {
    this->query = query;
    return *this;
}

Search& Search::set_georange(float latitude, float longitude, float distance) {
    this->latitude = latitude;
    this->longitude = longitude;
    this->distance = distance;
    geo_query = true;
    return *this;
}


Search& Search::set_range(int start, int end) {
    this->range_start = start;
    this->range_end = end;
    return *this;
}


Search::iterator Search::begin() const {
    if ( this->search_started ) {
        return new search_iterator::InternalData(this, internal->results.begin());
    }

    if ( ! mp_internalDb->hasDatabase() ) {
        if (verbose) {
          std::cout << "No database, no result" << std::endl;
        }
        estimated_matches_number = 0;
        return nullptr;
    }

    Xapian::QueryParser* queryParser = new Xapian::QueryParser();
    if (verbose) {
      std::cout << "Setup queryparser using language " << mp_internalDb->m_language << std::endl;
    }
    mp_internalDb->setupQueryparser(queryParser, suggestion_mode);

    std::string prefix = "";
    unsigned flags = Xapian::QueryParser::FLAG_DEFAULT;
    if (suggestion_mode) {
      if (verbose) {
        std::cout << "Mark query as 'partial'" << std::endl;
      }
      flags |= Xapian::QueryParser::FLAG_PARTIAL;
      if ( !mp_internalDb->m_hasNewSuggestionFormat
        && mp_internalDb->m_prefixes.find("S") != std::string::npos ) {
        if (verbose) {
          std::cout << "Searching in title namespace" << std::endl;
        }
        prefix = "S";
      }
    }
    Xapian::Query query;
    try {
      query = parse_query(queryParser, this->query, flags, prefix, suggestion_mode);
    } catch (Xapian::QueryParserError& e) {
      estimated_matches_number = 0;
      return nullptr;
    }
    if (verbose) {
        std::cout << "Parsed query '" << this->query << "' to " << query.get_description() << std::endl;
    }
    delete queryParser;

    Xapian::Enquire enquire(mp_internalDb->m_database);

    if (geo_query && mp_internalDb->hasValue("geo.position")) {
        Xapian::GreatCircleMetric metric;
        Xapian::LatLongCoord centre(latitude, longitude);
        Xapian::LatLongDistancePostingSource ps(mp_internalDb->valueSlot("geo.position"), centre, metric, distance);
        if ( this->query.empty()) {
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
    if (suggestion_mode) {
      enquire.set_weighting_scheme(Xapian::BM25Weight(0.001,0,1,1,0.5));
      if (mp_internalDb->hasValue("title")) {
        enquire.set_sort_by_relevance_then_value(mp_internalDb->valueSlot("title"), false);
      }
    }

    enquire.set_query(query);

    if (suggestion_mode && mp_internalDb->hasValue("targetPath")) {
      enquire.set_collapse_key(mp_internalDb->valueSlot("targetPath"));
    }

    internal->results = enquire.get_mset(this->range_start, this->range_end-this->range_start);
    search_started = true;
    estimated_matches_number = internal->results.get_matches_estimated();
    return new search_iterator::InternalData(this, internal->results.begin());
}

Search::iterator Search::end() const {
    if ( ! mp_internalDb->hasDatabase() ) {
        return nullptr;
    }
    return new search_iterator::InternalData(this, internal->results.end());
}

int Search::get_matches_estimated() const {
    // Ensure that the search as begin
    begin();
    return estimated_matches_number;
}

} //namespace zim
