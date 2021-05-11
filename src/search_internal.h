/*
 * Copyright (C) 2006 Tommi Maekitalo
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

#ifndef ZIM_SEARCH_INTERNAL_H
#define ZIM_SEARCH_INTERNAL_H

#include <xapian.h>

#include <zim/entry.h>

namespace zim {

/**
 * A class to encapsulate a xapian database and all the information we can gather from it.
 */
class InternalDataBase {
  public: // methods
    InternalDataBase(const std::vector<Archive>& archives, bool suggestionMode);
    bool hasDatabase() const;
    bool hasValuesmap() const;
    bool hasValue(const std::string& valueName) const;
    int  valueSlot(const std::string&  valueName) const;

    void setupQueryparser(Xapian::QueryParser* queryParse, bool suggestionMode);

  public: // data
    // The (main) database we will search on (wrapping other xapian databases).
    Xapian::Database m_database;

    // The real databases.
    std::vector<Xapian::Database> m_xapianDatabases;

    // The archives we are searching on.
    std::vector<Archive> m_archives;

    // The valuesmap associated with the database.
    std::map<std::string, int> m_valuesmap;

    // The prefix stored in the database.
    std::string m_prefixes;

    // If the database is open for suggestion.
    // True even if the dabase has no newSuggestionformat.
    bool m_suggestionMode;

    // If the database is open for suggestion and has new suggestion format (title db).
    bool m_hasNewSuggestionFormat;

    // The language of the database.
    std::string m_language;

    // The stop words associated to the language of the database.
    std::string m_stopwords;
};

struct Search::InternalData {
    InternalData(const std::vector<Archive>& archives, bool suggestionMode)
     : m_internalDb(archives, suggestionMode),
       results()
       {}
    InternalDataBase m_internalDb;
    Xapian::MSet results;
};

struct search_iterator::InternalData {
    std::shared_ptr<Search::InternalData> searchData;
    Xapian::MSetIterator iterator;
    Xapian::Document _document;
    bool document_fetched;
    std::unique_ptr<Entry> _entry;

    InternalData(const InternalData& other) :
      searchData(other.searchData),
      iterator(other.iterator),
      _document(other._document),
      document_fetched(other.document_fetched),
      _entry(other._entry ? new Entry(*other._entry) : nullptr )
    {
    }

    InternalData& operator=(const InternalData& other)
    {
      if (this != &other) {
        searchData = other.searchData;
        iterator = other.iterator;
        _document = other._document;
        document_fetched = other.document_fetched;
        _entry.reset(new Entry(*other._entry.get()));
      }
      return *this;
    }

    InternalData(const Search* search, Xapian::MSetIterator iterator) :
        searchData(search->internal),
        iterator(iterator),
        document_fetched(false)
    {};

    Xapian::Document get_document() {
        if ( !document_fetched ) {
            if (iterator != searchData->results.end()) {
                _document = iterator.get_document();
            }
            document_fetched = true;
        }
        return _document;
    }

    int get_databasenumber() {
        Xapian::docid docid = *iterator;
        return (docid - 1) % searchData->m_internalDb.m_archives.size();
    }

    Entry& get_entry() {
        if ( !_entry ) {
            int databasenumber = get_databasenumber();
            auto archive = searchData->m_internalDb.m_archives.at(databasenumber);
            _entry.reset(new Entry(archive.getEntryByPath(get_document().get_data())));
        }
        return *_entry.get();
    }
};



}; //namespace zim

#endif //ZIM_SEARCH_INTERNAL_H
