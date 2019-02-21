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
#include <zim/file.h>
#include "search_internal.h"
#include "levenshtein.h"
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

#if defined(ENABLE_XAPIAN)
#include "xapian.h"
#include <unicode/locid.h>
#endif

#define MAX_MATCHES_TO_SORT 10000

namespace zim
{

#if defined(ENABLE_XAPIAN)
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


void
setup_queryParser(Xapian::QueryParser* queryparser,
                  Xapian::Database& database,
                  const std::string& language,
                  const std::string& stopwords) {
    queryparser->set_database(database);
    if ( ! language.empty() )
    {
        /* Build ICU Local object to retrieve ISO-639 language code (from
           ISO-639-3) */
        icu::Locale languageLocale(language.c_str());

        /* Configuring language base steemming */
        try {
            Xapian::Stem stemmer = Xapian::Stem(languageLocale.getLanguage());
            queryparser->set_stemmer(stemmer);
            queryparser->set_stemming_strategy(Xapian::QueryParser::STEM_ALL);
        } catch (...) {
            std::cout << "No steemming for language '" << languageLocale.getLanguage() << "'" << std::endl;
        }
    }

    if ( ! stopwords.empty() )
    {
        std::string stopWord;
        std::istringstream file(stopwords);
        Xapian::SimpleStopper* stopper = new Xapian::SimpleStopper();
        while (std::getline(file, stopWord, '\n')) {
            stopper->add(stopWord);
        }
        stopper->release();
        queryparser->set_stopper(stopper);
    }
}

class LevenshteinDistanceMaker : public Xapian::KeyMaker {
  public:
    LevenshteinDistanceMaker(const std::string& query, size_t value_index):
        query(query),
        value_index(value_index) {}
    ~LevenshteinDistanceMaker() = default;

    virtual std::string operator() (const Xapian::Document &doc) const {
       auto document_value = doc.get_value(value_index);
       return Xapian::sortable_serialise(
                  levenshtein_distance(document_value, query));
    }
  private:
    std::string query;
    size_t value_index;
};

}
#endif

Search::Search(const std::vector<const File*> zimfiles) :
    internal(new InternalData),
    zimfiles(zimfiles),
    prefixes(""), query(""),
    latitude(0), longitude(0), distance(0),
    range_start(0), range_end(0),
    suggestion_mode(false),
    geo_query(false),
    search_started(false),
    has_database(false),
    verbose(false),
    estimated_matches_number(0)
{}

Search::Search(const File* zimfile) :
    internal(new InternalData),
    prefixes(""), query(""),
    latitude(0), longitude(0), distance(0),
    range_start(0), range_end(0),
    suggestion_mode(false),
    geo_query(false),
    search_started(false),
    has_database(false),
    verbose(false),
    estimated_matches_number(0)
{
    zimfiles.push_back(zimfile);
}

Search::Search(const Search& it) :
     internal(new InternalData),
     zimfiles(it.zimfiles),
     prefixes(it.prefixes),
     query(it.query),
     latitude(it.latitude), longitude(it.longitude), distance(it.distance),
     range_start(it.range_start), range_end(it.range_end),
     suggestion_mode(it.suggestion_mode),
     geo_query(it.geo_query),
     search_started(false),
     has_database(false),
     verbose(it.verbose),
     estimated_matches_number(0)
{ }

Search& Search::operator=(const Search& it)
{
     if ( internal ) internal.reset();
     zimfiles = it.zimfiles;
     prefixes = it.prefixes;
     query = it.query;
     latitude = it.latitude;
     longitude = it.longitude;
     distance = it.distance;
     range_start = it.range_start;
     range_end = it.range_end;
     suggestion_mode = it.suggestion_mode;
     geo_query = it.geo_query;
     search_started = false;
     has_database = false;
     verbose = it.verbose;
     estimated_matches_number = 0;
     return *this;
}

Search::Search(Search&& it) = default;
Search& Search::operator=(Search&& it) = default;
Search::~Search() = default;

void Search::set_verbose(bool verbose) {
    std::cout << "set verbose" << std::endl;
    this->verbose = verbose;
}

Search& Search::add_zimfile(const File* zimfile) {
    zimfiles.push_back(zimfile);
    return *this;
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

Search& Search::set_suggestion_mode(const bool suggestion_mode) {
    this->suggestion_mode = suggestion_mode;
    return *this;
}

Search::iterator Search::begin() const {
#if defined(ENABLE_XAPIAN)
    if ( this->search_started ) {
        return new search_iterator::InternalData(this, internal->results.begin());
    }

    std::vector<const File*>::const_iterator it;
    bool first = true;
    std::string language;
    std::string stopwords;
    for(it=zimfiles.begin(); it!=zimfiles.end(); it++)
    {
        const File* zimfile = *it;
        if (zimfile->is_multiPart()) {
            continue;
        }
        zim::Article xapianArticle = zimfile->getArticle('X', "fulltext/xapian");
        if (!xapianArticle.good()) {
          xapianArticle = zimfile->getArticle('Z', "/fulltextIndex/xapian");
        }
        if (!xapianArticle.good()) {
            continue;
        }
        auto dbOffset = xapianArticle.getOffset();
        if (dbOffset == 0) {
            continue;
        }
        DEFAULTFS::FD databasefd;
        try {
            databasefd = DEFAULTFS::openFile(zimfile->getFilename());
        } catch (...) {
            std::cerr << "Impossible to open " << zimfile->getFilename() << std::endl;
            std::cerr << strerror(errno) << std::endl;
            continue;
        }
        if (!databasefd.seek(offset_t(dbOffset))) {
            std::cerr << "Something went wrong seeking databasedb "
                      << zimfile->getFilename() << std::endl;
            std::cerr << "dbOffest = " << dbOffset << std::endl;
            continue;
        }
        Xapian::Database database;
        try {
            database = Xapian::Database(databasefd.release());
        } catch( Xapian::DatabaseError& e) {
            std::cerr << "Something went wrong opening xapian database for zimfile "
                      << zimfile->getFilename() << std::endl;
            std::cerr << "dbOffest = " << dbOffset << std::endl;
            std::cerr << "error = " << e.get_msg() << std::endl;
            continue;
        }

        if ( first ) {
            this->valuesmap = read_valuesmap(database.get_metadata("valuesmap"));
            language = database.get_metadata("language");
            if (language.empty() ) {
              // Database created before 2017/03 has no language metadata.
              // However, term were stemmed anyway and we need to stem our
              // search query the same the database was created.
              // So we need a language, let's use the one of the zim.
              // If zimfile has no language metadata, we can't do lot more here :/
              auto article = zimfile->getArticle('M', "Language");
              if ( article.good() ) {
                language = article.getData();
              }
            }
            stopwords = database.get_metadata("stopwords");
            this->prefixes = database.get_metadata("prefixes");
        } else {
            std::map<std::string, int> valuesmap = read_valuesmap(database.get_metadata("valuesmap"));
            if (this->valuesmap != valuesmap ) {
                // [TODO] Ignore the database, raise a error ?
            }
        }
        internal->xapian_databases.push_back(database);
        internal->database.add_database(database);
        has_database = true;
    }

    if ( ! has_database ) {
        if (verbose) {
          std::cout << "No database, no result" << std::endl;
        }
        estimated_matches_number = 0;
        return nullptr;
    }
    
    Xapian::QueryParser* queryParser = new Xapian::QueryParser();
    if (verbose) {
      std::cout << "Setup queryparser using language " << language << std::endl;
    }
    queryParser->set_default_op(Xapian::Query::op::OP_AND);
    setup_queryParser(queryParser, internal->database, language, stopwords);

    std::string prefix = "";
    unsigned flags = Xapian::QueryParser::FLAG_DEFAULT;
    if (suggestion_mode) {
      if (verbose) {
        std::cout << "Mark query as 'partial'" << std::endl;
      }
      flags |= Xapian::QueryParser::FLAG_PARTIAL;
      if (this->prefixes.find("S") != std::string::npos ) {
        if (verbose) {
          std::cout << "Searching in title namespace" << std::endl;
        }
        prefix = "S";
      }
    }
    Xapian::Query query;
    try {
      query = queryParser->parse_query(this->query, flags, prefix);
    } catch (Xapian::QueryParserError& e) {
      estimated_matches_number = 0;
      return nullptr;
    }
    if (verbose) {
        std::cout << "Parsed query '" << this->query << "' to " << query.get_description() << std::endl;
    }
    delete queryParser;
    
    Xapian::Enquire enquire(internal->database);
    Xapian::KeyMaker* keyMaker(nullptr);

    if (geo_query && valuesmap.find("geo.position") != valuesmap.end()) {
        Xapian::GreatCircleMetric metric;
        Xapian::LatLongCoord centre(latitude, longitude);
        Xapian::LatLongDistancePostingSource ps(valuesmap["geo.position"], centre, metric, distance);
        if ( this->query.empty()) {
          query = Xapian::Query(&ps);
        } else {
          query = Xapian::Query(Xapian::Query::OP_FILTER, query, Xapian::Query(&ps));
        }
    }

    enquire.set_query(query);

    if (suggestion_mode) {
      size_t value_index = 0;
      bool has_custom_distance_maker = true;
      if ( !valuesmap.empty() ) {
        if ( valuesmap.find("title") != valuesmap.end() ) {
          value_index = valuesmap["title"];
        } else {
          // This should not happen as valuesmap has a title entry, but let's
          // be tolerent.
          has_custom_distance_maker = false;
        }
      }
      auto temp_results = enquire.get_mset(0,0);
      if ( has_custom_distance_maker
        && temp_results.get_matches_estimated() <= MAX_MATCHES_TO_SORT ) {
        keyMaker = new LevenshteinDistanceMaker(this->query, value_index);
        enquire.set_sort_by_key(keyMaker, false);
      }
    }
    
    internal->results = enquire.get_mset(this->range_start, this->range_end-this->range_start);
    search_started = true;
    estimated_matches_number = internal->results.get_matches_estimated();
    delete keyMaker;
    return new search_iterator::InternalData(this, internal->results.begin());
#else
    estimated_matches_number = 0;
    return nullptr;
#endif
}

Search::iterator Search::end() const {
#if defined(ENABLE_XAPIAN)
    if ( ! has_database ) {
        return nullptr;
    }
    return new search_iterator::InternalData(this, internal->results.end());
#else
    return nullptr;
#endif
}

int Search::get_matches_estimated() const {
    // Ensure that the search as begin
    begin();
    return estimated_matches_number;
}

} //namespace zim
