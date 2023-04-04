/*
 * Copyright (C) 2021 Maneesh P M <manu.pm55@gmail.com>
 * Copyright (C) 2017-2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#define ZIM_PRIVATE

#include "xapian/myhtmlparse.h"
#include <zim/search_iterator.h>
#include <zim/search.h>
#include <zim/archive.h>
#include <zim/item.h>
#include "search_internal.h"

namespace zim {


SearchIterator::~SearchIterator() = default;
SearchIterator::SearchIterator(SearchIterator&& it) = default;
SearchIterator& SearchIterator::operator=(SearchIterator&& it) = default;

SearchIterator::SearchIterator() : SearchIterator(nullptr)
{};

SearchIterator::SearchIterator(InternalData* internal_data)
  : internal(internal_data)
{}

SearchIterator::SearchIterator(const SearchIterator& it)
    : internal(nullptr)
{
    if (it.internal) internal = std::unique_ptr<InternalData>(new InternalData(*it.internal));
}

SearchIterator & SearchIterator::operator=(const SearchIterator& it) {
    if ( ! it.internal ) internal.reset();
    else if ( ! internal ) internal = std::unique_ptr<InternalData>(new InternalData(*it.internal));
    else *internal = *it.internal;

    return *this;
}

bool SearchIterator::operator==(const SearchIterator& it) const {
    if ( ! internal && ! it.internal) {
        return true;
    }
    if ( ! internal || ! it.internal) {
        return false;
    }
    return (*internal == *it.internal);
}

bool SearchIterator::operator!=(const SearchIterator& it) const {
    return ! (*this == it);
}

SearchIterator& SearchIterator::operator++() {
    if ( ! internal ) {
        return *this;
    }
    ++(internal->_iterator);
    internal->document_fetched = false;
    internal->_entry.reset();
    return *this;
}

SearchIterator SearchIterator::operator++(int) {
    SearchIterator it = *this;
    operator++();
    return it;
}

SearchIterator& SearchIterator::operator--() {
    if ( ! internal ) {
        return *this;
    }
    --(internal->_iterator);
    internal->document_fetched = false;
    internal->_entry.reset();
    return *this;
}

SearchIterator SearchIterator::operator--(int) {
    SearchIterator it = *this;
    operator--();
    return it;
}

std::string SearchIterator::getPath() const {
    if ( ! internal ) {
        return "";
    }

    std::string path = internal->get_document().get_data();
    bool hasNewNamespaceScheme = internal->mp_internalDb->m_archives.at(getFileIndex()).hasNewNamespaceScheme();

    std::string dbDataType = internal->mp_internalDb->m_database.get_metadata("data");
    if (dbDataType.empty()) {
        dbDataType = "fullPath";
    }

    // If the archive has new namespace scheme and the type of its indexed data
    // is `fullPath` we return only the `path` without namespace
    if (hasNewNamespaceScheme && dbDataType == "fullPath") {
        path = path.substr(2);
    }
    return path;
}

std::string SearchIterator::getDbData() const {
    if ( ! internal ) {
        return "";
    }

    return internal->get_document().get_data();
}

std::string SearchIterator::getTitle() const {
    if ( ! internal ) {
        return "";
    }
    return internal->get_entry().getTitle();
}

int SearchIterator::getScore() const {
    if ( ! internal ) {
        return 0;
    }
    return internal->iterator().get_percent();
}

std::string SearchIterator::getSnippet() const {
    if ( ! internal ) {
        return "";
    }

    // Generate full text snippet
    if ( ! internal->mp_internalDb->hasValuesmap() )
    {
        /* This is the old legacy version. Guess and try */
        std::string stored_snippet = internal->get_document().get_value(1);
        if ( ! stored_snippet.empty() )
            return stored_snippet;
        /* Let's continue here, and see if we can genenate one */
    }
    else if ( internal->mp_internalDb->hasValue("snippet") )
    {
        return internal->get_document().get_value(internal->mp_internalDb->valueSlot("snippet"));
    }

    Entry& entry = internal->get_entry();
    /* No reader, no snippet */
    try {
        /* Get the content of the item to generate a snippet.
           We parse it and use the html dump to avoid remove html tags in the
           content and be able to nicely cut the text at random place. */
        zim::MyHtmlParser htmlParser;
        std::string content = entry.getItem().getData();
        try {
          htmlParser.parse_html(content, "UTF-8", true);
        } catch (...) {}
        return internal->mp_mset->snippet(htmlParser.dump,
                                          /*length=*/500,
                                          /*stemmer=*/internal->mp_internalDb->m_stemmer,
                                          /*flags=*/0);
    } catch (...) {
      return "";
    }
}

int SearchIterator::getSize() const {
    if ( ! internal ) {
        return -1;
    }
    if ( ! internal->mp_internalDb->hasValuesmap() )
    {
        /* This is the old legacy version. Guess and try */
        return internal->get_document().get_value(2).empty() == true ? -1 : atoi(internal->get_document().get_value(2).c_str());
    }
    else if ( internal->mp_internalDb->hasValue("size") )
    {
        return atoi(internal->get_document().get_value(internal->mp_internalDb->valueSlot("size")).c_str());
    }
    /* The size is never used. Do we really want to get the content and
       calculate the size ? */
    return -1;
}

int SearchIterator::getWordCount() const      {
    if ( ! internal ) {
        return -1;
    }
    if ( ! internal->mp_internalDb->hasValuesmap() )
    {
        /* This is the old legacy version. Guess and try */
        return internal->get_document().get_value(3).empty() == true ? -1 : atoi(internal->get_document().get_value(3).c_str());
    }
    else if ( internal->mp_internalDb->hasValue("wordcount") )
    {
        return atoi(internal->get_document().get_value(internal->mp_internalDb->valueSlot("wordcount")).c_str());
    }
    return -1;
}

int SearchIterator::getFileIndex() const {
    if ( internal ) {
        return internal->get_databasenumber();
    }
    return 0;
}

Uuid SearchIterator::getZimId() const {
    if (! internal ) {
        throw std::runtime_error("Cannot get zimId from uninitialized iterator");
    }
    return internal->mp_internalDb->m_archives.at(getFileIndex()).getUuid();
}

SearchIterator::reference SearchIterator::operator*() const {
    if (! internal ) {
        throw std::runtime_error("Cannot get a entry for a uninitialized iterator");
    }
    return internal->get_entry();
}

SearchIterator::pointer SearchIterator::operator->() const {
    return &**this;
}


} // namespace zim
