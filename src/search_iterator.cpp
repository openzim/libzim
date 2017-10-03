/*
 * Copyright (C) 2017 Matthieu Gautier
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

#include "xapian/myhtmlparse.h"
#include <zim/search_iterator.h>
#include <zim/search.h>
#include <zim/file.h>
#include "search_internal.h"

namespace zim {


search_iterator::~search_iterator() = default;
search_iterator::search_iterator(search_iterator&& it) = default;
search_iterator& search_iterator::operator=(search_iterator&& it) = default;

search_iterator::search_iterator() : search_iterator(nullptr)
{};

search_iterator::search_iterator(InternalData* internal_data)
  : internal(internal_data)
{}

search_iterator::search_iterator(const search_iterator& it)
    : internal(nullptr)
{
    if (it.internal) internal = std::unique_ptr<InternalData>(new InternalData(*it.internal));
}

search_iterator & search_iterator::operator=(const search_iterator& it) {
    if ( ! it.internal ) internal.reset();
    else if ( ! internal ) internal = std::unique_ptr<InternalData>(new InternalData(*it.internal));
    else *internal = *it.internal;

    return *this;
}

bool search_iterator::operator==(const search_iterator& it) const {
#if defined(ENABLE_XAPIAN)
    if ( ! internal && ! it.internal)
        return true;
    if ( ! internal || ! it.internal)
        return false;
    return (internal->search == it.internal->search
         && internal->iterator == it.internal->iterator);
#else
    // If there is no xapian, there is no search. There is only one iterator: end.
    // So all iterators are equal.
    return true;
#endif
}

bool search_iterator::operator!=(const search_iterator& it) const {
    return ! (*this == it);
}

search_iterator& search_iterator::operator++() {
#if defined(ENABLE_XAPIAN)
    if ( ! internal ) {
        return *this;
    }
    ++(internal->iterator);
    internal->document_fetched = false;
    internal->article_fetched = false;
#endif
    return *this;
}

search_iterator search_iterator::operator++(int) {
    search_iterator it = *this;
    operator++();
    return it;
}

search_iterator& search_iterator::operator--() {
#if defined(ENABLE_XAPIAN)
    if ( ! internal ) {
        return *this;
    }
    --(internal->iterator);
    internal->document_fetched = false;
    internal->article_fetched = false;
#endif
    return *this;
}

search_iterator search_iterator::operator--(int) {
    search_iterator it = *this;
    operator--();
    return it;
}

std::string search_iterator::get_url() const {
#if defined(ENABLE_XAPIAN)
    if ( ! internal ) {
        return "";
    }
    return internal->get_document().get_data();
#else
    return "";
#endif
}

std::string search_iterator::get_title() const {
#if defined(ENABLE_XAPIAN)
    if ( ! internal ) {
        return "";
    }
    if ( internal->search->valuesmap.empty() )
    {
        /* This is the old legacy version. Guess and try */
        return internal->get_document().get_value(0);
    }
    else if ( internal->search->valuesmap.find("title") != internal->search->valuesmap.end() )
    {
        return internal->get_document().get_value(internal->search->valuesmap["title"]);
    }
#endif
    return "";
}

int search_iterator::get_score() const {
#if defined(ENABLE_XAPIAN)
    if ( ! internal ) {
        return 0;
    }
    return internal->iterator.get_percent();
#else
    return 0;
#endif
}

std::string search_iterator::get_snippet() const {
#if defined(ENABLE_XAPIAN)
    if ( ! internal ) {
        return "";
    }
    if ( internal->search->valuesmap.empty() )
    {
        /* This is the old legacy version. Guess and try */
        std::string stored_snippet = internal->get_document().get_value(1);
        if ( ! stored_snippet.empty() )
            return stored_snippet;
        /* Let's continue here, and see if we can genenate one */
    }
    else if ( internal->search->valuesmap.find("snippet") != internal->search->valuesmap.end() )
    {
        return internal->get_document().get_value(internal->search->valuesmap["snippet"]);
    }
    /* No reader, no snippet */
    Article& article = internal->get_article();
    if ( ! article.good() )
        return "";
    /* Get the content of the article to generate a snippet.
       We parse it and use the html dump to avoid remove html tags in the
       content and be able to nicely cut the text at random place. */
    zim::MyHtmlParser htmlParser;
    std::string content = article.getData();
    try {
        htmlParser.parse_html(content, "UTF-8", true);
    } catch (...) {}
    return internal->search->internal->results.snippet(htmlParser.dump, 500);
#else
    return "";
#endif
}

int search_iterator::get_size() const {
#if defined(ENABLE_XAPIAN)
    if ( ! internal ) {
        return -1;
    }
    if ( internal->search->valuesmap.empty() )
    {
        /* This is the old legacy version. Guess and try */
        return internal->get_document().get_value(2).empty() == true ? -1 : atoi(internal->get_document().get_value(2).c_str());
    }
    else if ( internal->search->valuesmap.find("size") != internal->search->valuesmap.end() )
    {
        return atoi(internal->get_document().get_value(internal->search->valuesmap["size"]).c_str());
    }
#endif
    /* The size is never used. Do we really want to get the content and
       calculate the size ? */
    return -1;
}

int search_iterator::get_wordCount() const      {
#if defined(ENABLE_XAPIAN)
    if ( ! internal ) {
        return -1;
    }
    if ( internal->search->valuesmap.empty() )
    {
        /* This is the old legacy version. Guess and try */
        return internal->get_document().get_value(3).empty() == true ? -1 : atoi(internal->get_document().get_value(3).c_str());
    }
    else if ( internal->search->valuesmap.find("wordcount") != internal->search->valuesmap.end() )
    {
        return atoi(internal->get_document().get_value(internal->search->valuesmap["wordcount"]).c_str());
    }
#endif
    return -1;
}

int search_iterator::get_fileIndex() const {
#if defined(ENABLE_XAPIAN)
    if ( internal ) {
        return internal->get_databasenumber();
    }
#endif
    return 0;
}

search_iterator::reference search_iterator::operator*() const {
    return internal->get_article();
}

search_iterator::pointer search_iterator::operator->() const {
    return &internal->get_article();
}

} // namespace zim
