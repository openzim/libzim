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

#include "config.h"

#if defined(ENABLE_XAPIAN)
#include <xapian.h>
#endif

#include <zim/entry.h>

namespace zim {

struct Search::InternalData {
#if defined(ENABLE_XAPIAN)
    std::vector<Xapian::Database> xapian_databases;
    Xapian::Database database;
    Xapian::MSet results;
#endif
};

struct search_iterator::InternalData {
#if defined(ENABLE_XAPIAN)
    const Search* search;
    Xapian::MSetIterator iterator;
    Xapian::Document _document;
    bool document_fetched;
#endif
    std::unique_ptr<Entry> _entry;

#if defined(ENABLE_XAPIAN)
    InternalData(const InternalData& other) :
      search(other.search),
      iterator(other.iterator),
      _document(other._document),
      document_fetched(other.document_fetched),
      _entry(other._entry ? new Entry(*other._entry) : nullptr )
    {
    }

    InternalData& operator=(const InternalData& other)
    {
      if (this != &other) {
        search = other.search;
        iterator = other.iterator;
        _document = other._document;
        document_fetched = other.document_fetched;
        _entry.reset(new Entry(*other._entry.get()));
      }
      return *this;
    }

    InternalData(const Search* search, Xapian::MSetIterator iterator) :
        search(search),
        iterator(iterator),
        document_fetched(false)
    {};

    Xapian::Document get_document() {
        if ( !document_fetched ) {
            if (iterator != search->internal->results.end()) {
                _document = iterator.get_document();
            }
            document_fetched = true;
        }
        return _document;
    }
#endif

    int get_databasenumber() {
#if defined(ENABLE_XAPIAN)
        Xapian::docid docid = *iterator;
        return (docid - 1) % search->m_archives.size();
#endif
        return 0;
    }

    Entry& get_entry() {
#if defined(ENABLE_XAPIAN)
        if ( !_entry ) {
            int databasenumber = get_databasenumber();
            auto archive = search->m_archives.at(databasenumber);
            _entry.reset(new Entry(archive.getEntryByPath(get_document().get_data())));
        }
#endif
        return *_entry.get();
    }
};



}; //namespace zim

#endif //ZIM_SEARCH_INTERNAL_H
