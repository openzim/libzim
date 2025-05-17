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
#include "./smartptr.h"

#include <stdexcept>

namespace zim
{

#if defined(LIBZIM_WITH_XAPIAN)
struct SuggestionIterator::SuggestionInternalData {
    std::shared_ptr<SuggestionDataBase> mp_internalDb;
    std::shared_ptr<Xapian::MSet> mp_mset;
    Xapian::MSetIterator iterator;
    Xapian::Document _document;
    bool document_fetched;
    ValuePtr<Entry> _entry;

    SuggestionInternalData(std::shared_ptr<SuggestionDataBase> p_internalDb, std::shared_ptr<Xapian::MSet> p_mset, Xapian::MSetIterator iterator) :
        mp_internalDb(p_internalDb),
        mp_mset(p_mset),
        iterator(iterator),
        document_fetched(false)
    {};

    Xapian::Document get_document() {
        if ( !document_fetched ) {
            if (iterator == mp_mset->end()) {
                throw std::runtime_error("Cannot get entry for end iterator");
            }
            _document = iterator.get_document();
            document_fetched = true;
        }
        return _document;
    }

    Entry& get_entry() {
        if (!_entry) {
            _entry.reset(new Entry(mp_internalDb->m_archive.getEntryByPath(get_document().get_data())));
        }
        return *_entry.get();
    }

    bool operator==(const SuggestionInternalData& other) const {
        return (mp_internalDb == other.mp_internalDb
            &&  mp_mset == other.mp_mset
            &&  iterator == other.iterator);
    }

    std::string getIndexPath();
    std::string getIndexTitle();
    std::string getIndexSnippet();
};

std::string SuggestionIterator::SuggestionInternalData::getIndexPath()
{
    try {
        std::string path = get_document().get_data();
        bool hasNewNamespaceScheme = mp_internalDb->m_archive.hasNewNamespaceScheme();

        std::string dbDataType = mp_internalDb->m_database.get_metadata("data");
        if (dbDataType.empty()) {
            dbDataType = "fullPath";
        }

        // If the archive has new namespace scheme and the type of its indexed
        // data is `fullPath` we return only the `path` without namespace
        if (hasNewNamespaceScheme && dbDataType == "fullPath") {
            path = path.substr(2);
        }
        return path;
    } catch ( Xapian::DatabaseError& e) {
        throw ZimFileFormatError(e.get_description());
    }
}

std::string SuggestionIterator::SuggestionInternalData::getIndexTitle() {
    try {
        return get_entry().getTitle();
    } catch (...) {
        return "";
    }
}

std::string SuggestionIterator::SuggestionInternalData::getIndexSnippet() {
    try {
        return mp_mset->snippet(getIndexTitle(), 500, mp_internalDb->m_stemmer);
    } catch(...) {
        return "";
    }
}
#endif  // LIBZIM_WITH_XAPIAN

SuggestionIterator::~SuggestionIterator() = default;
SuggestionIterator::SuggestionIterator(SuggestionIterator&& it) = default;
SuggestionIterator& SuggestionIterator::operator=(SuggestionIterator&& it) = default;

SuggestionIterator::SuggestionIterator(RangeIterator rangeIterator)
  : mp_rangeIterator(new RangeIterator(rangeIterator))
{}

#if defined(LIBZIM_WITH_XAPIAN)
SuggestionIterator::SuggestionIterator(SuggestionInternalData* internal)
  : mp_internal(internal)
{}
#endif  // LIBZIM_WITH_XAPIAN

SuggestionIterator::SuggestionIterator(const SuggestionIterator& it)
{
#if defined(LIBZIM_WITH_XAPIAN)
    if (it.mp_internal) {
        mp_internal.reset(new SuggestionInternalData(*it.mp_internal));
    }
#endif  // LIBZIM_WITH_XAPIAN

    if (it.mp_rangeIterator) {
        mp_rangeIterator.reset(new RangeIterator(*it.mp_rangeIterator));
    }
}

SuggestionIterator& SuggestionIterator::operator=(const SuggestionIterator& it) {
    if ( this != &it ) {
      this->~SuggestionIterator();
      new(this) SuggestionIterator(it);
    }
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
        return **mp_rangeIterator;
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

#endif  // LIBZIM_WITH_XAPIAN

SuggestionItem* SuggestionIterator::instantiateSuggestion() const
{
#if defined(LIBZIM_WITH_XAPIAN)
    if (mp_internal) {
        return new SuggestionItem(mp_internal->getIndexTitle(),
                                  mp_internal->getIndexPath(),
                                  mp_internal->getIndexSnippet());
    }
#endif  // LIBZIM_WITH_XAPIAN

    if (mp_rangeIterator) {
        return new SuggestionItem((*mp_rangeIterator)->getTitle(),
                                  (*mp_rangeIterator)->getPath());
    }

    return nullptr;
}

const SuggestionItem& SuggestionIterator::operator*() {
    if (!m_suggestionItem) {
      m_suggestionItem.reset(instantiateSuggestion());

      if (!m_suggestionItem){
          throw std::runtime_error("Cannot dereference iterator");
      }
    }
    return *m_suggestionItem;
}

const SuggestionItem* SuggestionIterator::operator->() {
    operator*();
    return m_suggestionItem.get();
}

////////////////////////////////////////////////////////////////////////////////
// SuggestionIterator related methods of SuggestionResultSet
////////////////////////////////////////////////////////////////////////////////

SuggestionResultSet::iterator SuggestionResultSet::begin() const
{
#if defined(LIBZIM_WITH_XAPIAN)
    if ( ! mp_entryRange ) {
        return iterator(new iterator::SuggestionInternalData(mp_internalDb, mp_mset, mp_mset->begin()));
    }
#endif  // LIBZIM_WITH_XAPIAN

    return iterator(mp_entryRange->begin());
}

SuggestionResultSet::iterator SuggestionResultSet::end() const
{
#if defined(LIBZIM_WITH_XAPIAN)
    if ( ! mp_entryRange ) {
        return iterator(new iterator::SuggestionInternalData(mp_internalDb, mp_mset, mp_mset->end()));
    }
#endif  // LIBZIM_WITH_XAPIAN

    return iterator(mp_entryRange->end());
}

} // namespace zim
