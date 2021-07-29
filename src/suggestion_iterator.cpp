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

#define ZIM_PRIVATE

#include "zim/suggestion_iterator.h"
#include "suggestion_internal.h"

namespace zim
{

SuggestionIterator::~SuggestionIterator() = default;
SuggestionIterator::SuggestionIterator(SuggestionIterator&& it) = default;
SuggestionIterator& SuggestionIterator::operator=(SuggestionIterator&& it) = default;

SuggestionIterator::SuggestionIterator()
  : mp_rangeIterator(nullptr),
    mp_internal(nullptr)
{}

SuggestionIterator::SuggestionIterator(RangeIterator rangeIterator)
  : mp_rangeIterator(std::unique_ptr<RangeIterator>(new RangeIterator(rangeIterator))),
    mp_internal(nullptr)
{}

SuggestionIterator::SuggestionIterator(SuggestionInternalData* internal)
  : mp_rangeIterator(nullptr),
    mp_internal(internal)
{}

SuggestionIterator::SuggestionIterator(const SuggestionIterator& it)
    : mp_rangeIterator(nullptr), mp_internal(nullptr)
{
    if (it.mp_internal) {
        mp_internal = std::unique_ptr<SuggestionInternalData>(new SuggestionInternalData(*it.mp_internal));
    } else if (it.mp_rangeIterator) {
        mp_rangeIterator = std::unique_ptr<RangeIterator>(new RangeIterator(*it.mp_rangeIterator));
    }
}

SuggestionIterator& SuggestionIterator::operator=(const SuggestionIterator& it) {
    mp_rangeIterator.reset();
    if (it.mp_rangeIterator) {
        mp_rangeIterator.reset(new RangeIterator(*it.mp_rangeIterator));
    }

    mp_internal.reset();
    if (it.mp_internal) {
        mp_internal.reset(new SuggestionInternalData(*it.mp_internal));
    }

    m_suggestionItem.reset();
    return *this;
}

bool SuggestionIterator::operator==(const SuggestionIterator& it) const {
    if (mp_rangeIterator && it.mp_rangeIterator) {
        return (*mp_rangeIterator == *it.mp_rangeIterator);
    } else if (mp_internal && it.mp_internal) {
        return (*mp_internal == *it.mp_internal);
    }
    return false;
}

bool SuggestionIterator::operator!=(const SuggestionIterator& it) const {
    return ! (*this == it);
}

SuggestionIterator& SuggestionIterator::operator++() {
    if (mp_internal) {
        ++(mp_internal->iterator);
        mp_internal->_entry.reset();
        mp_internal->document_fetched = false;
    } else if (mp_rangeIterator) {
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
    if (mp_internal) {
        --(mp_internal->iterator);
        mp_internal->_entry.reset();
        mp_internal->document_fetched = false;
    } else if (mp_rangeIterator) {
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

std::string SuggestionIterator::getDbData() const {
    if (! mp_internal) {
        return "";
    }

    return mp_internal->get_document().get_data();
}

Entry SuggestionIterator::getEntry() const {
    if (mp_internal) {
        return mp_internal->get_entry();
    } else if (mp_rangeIterator) {
        return **mp_rangeIterator;
    }
    throw std::runtime_error("Cannot dereference iterator");
}

std::string SuggestionIterator::getIndexPath() const
{
    if (! mp_internal) {
        return "";
    }

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

const SuggestionItem& SuggestionIterator::operator*() {
    if (m_suggestionItem) {
        return *m_suggestionItem;
    }

    if (mp_internal) {
        m_suggestionItem.reset(new SuggestionItem(getIndexTitle(),
                getIndexPath(), getIndexSnippet()));
    } else if (mp_rangeIterator) {
        m_suggestionItem.reset(new SuggestionItem((*mp_rangeIterator)->getTitle(),
                                                (*mp_rangeIterator)->getPath()));
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
