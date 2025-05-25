/*
 * Copyright (C) 2021 Manneesh P M <manu.pm55@gmail.com>
 * Copyright (C) 2017-2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#include "tools.h"
#include "lock.h"
#include <mutex>
#include <xapian.h>

#include <zim/archive.h>
#include <zim/search.h>
#include <zim/entry.h>
#include <zim/error.h>

namespace zim {

class XapianDbMetadata {
  public: // methods
    XapianDbMetadata() = default;
    XapianDbMetadata(const Xapian::Database& db, std::string defaultLanguage);

    // Return a newly allocated stopper.
    // This stopper can (and should to be properly deleted) be directly passed to xapian
    // (queryparser or TermGenerator)
    Xapian::Stopper* new_stopper();

    bool hasValuesmap() const {
        return !m_valuesmap.empty();
    }

    bool hasValue(const std::string& valueName) const {
        return (m_valuesmap.find(valueName) != m_valuesmap.end());
    }

    int valueSlot(const std::string& valueName) const {
        return m_valuesmap.at(valueName);
    }

  public: // data
    // The valuesmap associated with the database.
    std::map<std::string, int> m_valuesmap;

    // The language of the database
    std::string m_language;

    // The stemmer associated to the language
    Xapian::Stem m_stemmer;

    // The stop words stored in the database
    std::string m_stopwords;
};

class XapianDb {
  public: // method
    XapianDb(const Xapian::Database& db, std::string defaultLanguage);

  public: // data
    XapianDbMetadata m_metadata;

    Xapian::Database m_db;

    std::recursive_mutex m_mutex;
};

/**
 * A class to encapsulate a xapian database and all the information we can gather from it.
 */
class InternalDataBase {
  public: // methods
    InternalDataBase(const std::vector<zim::Archive>& archives, bool verbose);
    bool hasDatabase() const;
    bool hasValuesmap() const;
    bool hasValue(const std::string& valueName) const;
    int  valueSlot(const std::string&  valueName) const;

    Xapian::Query parseQuery(const zim::Query& query);

    std::lock_guard<MultiMutex> lock();

  public: // data
    // The (main) database we will search on (wrapping other xapian databases).
    Xapian::Database m_database;

    // The archives we are searching on.
    std::vector<Archive> m_archives;

    // If the database is open for suggestion.
    // True even if the dabase has no newSuggestionformat.
    bool m_suggestionMode;

    // The query parser corresponding to the database.
    Xapian::QueryParser m_queryParser;

    // The metadata of the db
    XapianDbMetadata m_metadata;

    // The MultiMutex associated to the multi db
    MultiMutex m_mutexes;

    // Verbosity of operations.
    bool m_verbose;
};

struct SearchIterator::InternalData {
    std::shared_ptr<InternalDataBase> mp_internalDb;
    std::shared_ptr<Xapian::MSet> mp_mset;
    Xapian::MSetIterator _iterator;
    Xapian::Document _document;
    bool document_fetched;
    std::unique_ptr<Entry> _entry;

    InternalData(const InternalData& other) :
      mp_internalDb(other.mp_internalDb),
      mp_mset(other.mp_mset),
      _iterator(other._iterator),
      _document(other._document),
      document_fetched(other.document_fetched),
      _entry(other._entry ? new Entry(*other._entry) : nullptr )
    {
    }

    InternalData& operator=(const InternalData& other)
    {
      if (this != &other) {
        mp_internalDb = other.mp_internalDb;
        mp_mset = other.mp_mset;
        _iterator = other._iterator;
        _document = other._document;
        document_fetched = other.document_fetched;
        _entry.reset(other._entry ? new Entry(*other._entry) : nullptr);
      }
      return *this;
    }

    InternalData(std::shared_ptr<InternalDataBase> p_internalDb, std::shared_ptr<Xapian::MSet> p_mset, Xapian::MSetIterator iterator) :
        mp_internalDb(p_internalDb),
        mp_mset(p_mset),
        _iterator(iterator),
        document_fetched(false)
    {};

    Xapian::Document get_document() {
        try {
            if ( !document_fetched ) {
                _document = iterator().get_document();
                document_fetched = true;
            }
            return _document;
        } catch ( Xapian::DatabaseError& e) {
            throw zim::ZimFileFormatError(e.get_description());
        }
    }

    int get_databasenumber() {
        try {
            Xapian::docid docid = *iterator();
            return (docid - 1) % mp_internalDb->m_archives.size();
        } catch ( Xapian::DatabaseError& e) {
            throw zim::ZimFileFormatError(e.get_description());
        }
    }

    Entry& get_entry() {
        try {
            if ( !_entry ) {
                int databasenumber = get_databasenumber();
                auto archive = mp_internalDb->m_archives.at(databasenumber);
                _entry.reset(new Entry(archive.getEntryByPath(get_document().get_data())));
            }
        return *_entry.get();
        } catch ( Xapian::DatabaseError& e) {
            throw zim::ZimFileFormatError(e.get_description());
        }
    }

    bool operator==(const InternalData& other) const {
        return (mp_internalDb == other.mp_internalDb
            &&  mp_mset == other.mp_mset
            &&  _iterator == other._iterator);
    }

    bool is_end() const {
        return _iterator == mp_mset->end();
    }

    const Xapian::MSetIterator& iterator() const {
        if (is_end()) {
            throw std::runtime_error("Cannot get entry for end iterator");
        }
        return _iterator;
    }
};


#define LOCK_SEARCH(InternalDataBase) auto&& lock = (InternalDataBase)->lock(); (void) lock;

}; //namespace zim

#endif //ZIM_SEARCH_INTERNAL_H
