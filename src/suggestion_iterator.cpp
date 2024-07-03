/*
 * Copyright (C) 2021 Maneesh P M <manu.pm55@gmail.com>
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
#define ZIM_PRIVATE

#include "zim/suggestion_iterator.h"
#include "suggestion_internal.h"
#include <stdexcept>

namespace zim
{

SuggestionIterator::~SuggestionIterator() = default;
SuggestionIterator::SuggestionIterator(SuggestionIterator&& it) = default;
SuggestionIterator& SuggestionIterator::operator=(SuggestionIterator&& it) = default;

SuggestionIterator::SuggestionIterator(RangeIterator rangeIterator)
  : mp_rangeIterator(std::unique_ptr<RangeIterator>(new RangeIterator(rangeIterator)))
#if defined(LIBZIM_WITH_XAPIAN)
    , mp_internal(nullptr)
#endif  // LIBZIM_WITH_XAPIAN
{}

#if defined(LIBZIM_WITH_XAPIAN)
SuggestionIterator::SuggestionIterator(SuggestionInternalData* internal)
  : mp_rangeIterator(nullptr),
    mp_internal(internal)
{}
#endif  // LIBZIM_WITH_XAPIAN

SuggestionIterator::SuggestionIterator(const SuggestionIterator& it)
    : mp_rangeIterator(nullptr)
{
#if defined(LIBZIM_WITH_XAPIAN)
    mp_internal.reset(nullptr);
    if (it.mp_internal) {
        mp_internal = std::unique_ptr<SuggestionInternalData>(new SuggestionInternalData(*it.mp_internal));
    }
#endif  // LIBZIM_WITH_XAPIAN

    if (it.mp_rangeIterator) {
        mp_rangeIterator = std::unique_ptr<RangeIterator>(new RangeIterator(*it.mp_rangeIterator));
    }
}

SuggestionIterator& SuggestionIterator::operator=(const SuggestionIterator& it) {
    mp_rangeIterator.reset();
    if (it.mp_rangeIterator) {
        mp_rangeIterator.reset(new RangeIterator(*it.mp_rangeIterator));
    }

#if defined(LIBZIM_WITH_XAPIAN)
    mp_internal.reset();
    if (it.mp_internal) {
        mp_internal.reset(new SuggestionInternalData(*it.mp_internal));
    }
#endif  // LIBZIM_WITH_XAPIAN

    m_suggestionItem.reset();
    return *this;
}

bool SuggestionIterator::operator==(const SuggestionIterator& it) const {
    if (mp_rangeIterator && it.mp_rangeIterator) {
        return (*mp_rangeIterator == *it.mp_rangeIterator);
    }

#if defined(LIBZIM_WITH_XAPIAN)
    if (mp_internal && it.mp_internal) {
        return (*mp_internal == *it.mp_internal);
    }
#endif  // LIBZIM_WITH_XAPIAN

    return false;
}

bool SuggestionIterator::operator!=(const SuggestionIterator& it) const {
    return ! (*this == it);
}

SuggestionIterator& SuggestionIterator::operator++() {
#if defined(LIBZIM_WITH_XAPIAN)
    if (mp_internal) {
        ++(mp_internal->iterator);
        mp_internal->_entry.reset();
        mp_internal->document_fetched = false;
    }
#endif  // LIBZIM_WITH_XAPIAN

    if (mp_rangeIterator) {
        ++(*mp_rangeIterator);
    }
    m_suggestionItem.reset();
    return *this;
}

SuggestionIterator SuggestionIterator::operator++(int) {
    SuggestionIterator it = *this;
    operator++();
    return it;
}

SuggestionIterator& SuggestionIterator::operator--() {
#if defined(LIBZIM_WITH_XAPIAN)
    if (mp_internal) {
        --(mp_internal->iterator);
        mp_internal->_entry.reset();
        mp_internal->document_fetched = false;
    }
#endif  // LIBZIM_WITH_XAPIAN

    if (mp_rangeIterator) {
        --(*mp_rangeIterator);
    }
    m_suggestionItem.reset();
    return *this;
}

SuggestionIterator SuggestionIterator::operator--(int) {
    SuggestionIterator it = *this;
    operator--();
    return it;
}

Entry SuggestionIterator::getEntry() const {
#if defined(LIBZIM_WITH_XAPIAN)
    if (mp_internal) {
        try {
            return mp_internal->get_entry();
        } catch ( Xapian::DatabaseError& e) {
            throw ZimFileFormatError(e.get_description());
        }
    }
#endif  // LIBZIM_WITH_XAPIAN

    if (mp_rangeIterator) {
        try {
            return **mp_rangeIterator;
        } catch(const std::out_of_range& e) {
            // Archive will throw a out_of_range is iterator is end.
            // But xapian version will throw a std::runtime_error, let's throw the same exception
            // for consistent api
            throw std::runtime_error(e.what());
        }
    }
    throw std::runtime_error("Cannot dereference iterator");
}

#if defined(LIBZIM_WITH_XAPIAN)
std::string SuggestionIterator::getDbData() const {
    if (! mp_internal) {
        return "";
    }

    try {
        return mp_internal->get_document().get_data();
    } catch ( Xapian::DatabaseError& e) {
        throw ZimFileFormatError(e.get_description());
    }
}

std::string SuggestionIterator::getIndexPath() const
{
    if (! mp_internal) {
        return "";
    }

    try {
        std::string path = mp_internal->get_document().get_data();
        bool hasNewNamespaceScheme = mp_internal->mp_internalDb->m_archive.hasNewNamespaceScheme();

        std::string dbDataType = mp_internal->mp_internalDb->m_database.get_metadata("data");
        if (dbDataType.empty()) {
            dbDataType = "fullPath";
        }

        // If the archive has new namespace scheme and the type of its indexed data
        // is `fullPath` we return only the `path` without namespace
        if (hasNewNamespaceScheme && dbDataType == "fullPath") {
            path = path.substr(2);
        }
        return path;
    } catch ( Xapian::DatabaseError& e) {
        throw ZimFileFormatError(e.get_description());
    }
}

std::string SuggestionIterator::getIndexTitle() const {
    if ( ! mp_internal) {
        return "";
    }
    try {
        return mp_internal->get_entry().getTitle();
    } catch (...) {
        return "";
    }
}

std::string SuggestionIterator::getIndexSnippet() const {
    if (! mp_internal) {
        return "";
    }

    try {
        return mp_internal->mp_mset->snippet(getIndexTitle(), 500, mp_internal->mp_internalDb->m_stemmer);
    } catch(...) {
        return "";
    }
}
#endif  // LIBZIM_WITH_XAPIAN

const SuggestionItem& SuggestionIterator::operator*() {
    if (m_suggestionItem) {
        return *m_suggestionItem;
    }

#if defined(LIBZIM_WITH_XAPIAN)
    if (mp_internal) {
        m_suggestionItem.reset(new SuggestionItem(getIndexTitle(),
                getIndexPath(), getIndexSnippet()));
    } else
#endif  // LIBZIM_WITH_XAPIAN

    if (mp_rangeIterator) {
        try {
          m_suggestionItem.reset(new SuggestionItem((*mp_rangeIterator)->getTitle(),
                                                    (*mp_rangeIterator)->getPath()));
        } catch(const std::out_of_range& e) {
            // Archive will throw a out_of_range is iterator is end.
            // But xapian version will throw a std::runtime_error, let's throw the same exception
            // for consistent api
            throw std::runtime_error(e.what());
        }
    }

    if (!m_suggestionItem){
        throw std::runtime_error("Cannot dereference iterator");
    }

    return *m_suggestionItem.get();
}

const SuggestionItem* SuggestionIterator::operator->() {
    operator*();
    return m_suggestionItem.get();
}

} // namespace zim
