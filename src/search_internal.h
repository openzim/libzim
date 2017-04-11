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
    Article _article;
    bool article_fetched;


#if defined(ENABLE_XAPIAN)
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


    Article& get_article() {
#if defined(ENABLE_XAPIAN)
        if ( !article_fetched ) {
            Xapian::docid docid = *iterator;
            int databasenumber = (docid - 1) % search->zimfiles.size();
            const File* file = search->zimfiles[databasenumber];
            if ( ! file )
                _article = Article();
            else
                _article = file->getArticleByUrl(get_document().get_data());
            article_fetched = true;
        }
#endif
        return _article;
    }
};



}; //namespace zim

#endif //ZIM_SEARCH_INTERNAL_H
