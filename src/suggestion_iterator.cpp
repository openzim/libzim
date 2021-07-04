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

#include "../include/zim/suggestion_iterator.h"
#include "../include/zim/search_iterator.h"

namespace zim
{

SuggestionIterator::~SuggestionIterator() = default;
SuggestionIterator::SuggestionIterator(SuggestionIterator&& it) = default;
SuggestionIterator& SuggestionIterator::operator=(SuggestionIterator&& it) = default;

SuggestionIterator::SuggestionIterator()
  : mp_rangeIterator(nullptr),
    mp_searchIterator(nullptr)
{}

SuggestionIterator::SuggestionIterator(RangeIterator rangeIterator)
  : mp_rangeIterator(std::unique_ptr<RangeIterator>(new RangeIterator(rangeIterator))),
    mp_searchIterator(nullptr)
{}

SuggestionIterator::SuggestionIterator(SearchIterator searchIterator)
  : mp_rangeIterator(nullptr),
    mp_searchIterator(std::unique_ptr<SearchIterator>(new SearchIterator(searchIterator)))
{}

SuggestionIterator::SuggestionIterator(const SuggestionIterator& it)
    : mp_rangeIterator(nullptr), mp_searchIterator(nullptr)
{
    if (it.mp_searchIterator) {
        mp_searchIterator = std::unique_ptr<SearchIterator>(new SearchIterator(*it.mp_searchIterator));
    }
    else if (it.mp_rangeIterator) {
        mp_rangeIterator = std::unique_ptr<RangeIterator>(new RangeIterator(*it.mp_rangeIterator));
    }
}

SuggestionIterator & SuggestionIterator::operator=(const SuggestionIterator& it) {
    mp_rangeIterator.reset();
    if (it.mp_rangeIterator) {
        mp_rangeIterator.reset(new RangeIterator(*it.mp_rangeIterator));
    }

    mp_searchIterator.reset();
    if (it.mp_searchIterator) {
        mp_searchIterator.reset(new SearchIterator(*it.mp_searchIterator));
    }

    m_suggestionItem.reset();
    return *this;
}

bool SuggestionIterator::operator==(const SuggestionIterator& it) const {
    if (mp_rangeIterator && it.mp_rangeIterator) {
        return (*mp_rangeIterator == *it.mp_rangeIterator);
    } else if (mp_searchIterator && it.mp_searchIterator) {
        return (*mp_searchIterator == *it.mp_searchIterator);
    }
    return false;
}

bool SuggestionIterator::operator!=(const SuggestionIterator& it) const {
    return ! (*this == it);
}

SuggestionIterator& SuggestionIterator::operator++() {
    if (mp_searchIterator) {
        ++(*mp_searchIterator);
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
    if (mp_searchIterator) {
        --(*mp_searchIterator);
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
    if (! mp_searchIterator) {
        return "";
    }

    return mp_searchIterator->getDbData();
}

const SuggestionItem& SuggestionIterator::operator*() {
    if (m_suggestionItem) {
        return *m_suggestionItem;
    }

    if (mp_searchIterator) {
        m_suggestionItem.reset(new SuggestionItem((*mp_searchIterator)->getTitle(),
                mp_searchIterator->getPath(), mp_searchIterator->getSnippet()));
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
