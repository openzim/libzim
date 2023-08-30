/*
 * Copyright (C) 2021 Maneesh P M <manu.pm55@gmail.com>
 * Copyright (C) 2020 Matthieu Gautier <mgautier@kymeria.fr>
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

#ifndef ZIM_SEARCH_ITERATOR_H
#define ZIM_SEARCH_ITERATOR_H

#include <memory>
#include <iterator>
#include "entry.h"
#include "archive.h"
#include "uuid.h"

namespace zim
{
class SearchResultSet;

/**
 * A interator on search result (an Entry)
 *
 * Be aware that the referenced/pointed Entry is generated and stored
 * in the iterator itself.
 * Once the iterator is destructed or incremented/decremented, you must NOT
 * use the Entry.
 */
class LIBZIM_API SearchIterator
{
    friend class zim::SearchResultSet;
    public:
        /* SuggestionIterator is conceptually a bidirectional iterator.
         * But std *LegayBidirectionalIterator* is also a *LegacyForwardIterator* and
         * it would impose us that :
         * > Given a and b, dereferenceable iterators of type It:
         * >  If a and b compare equal (a == b is contextually convertible to true)
         * >  then either they are both non-dereferenceable or *a and *b are references bound to the same object.
         * and
         * > the LegacyForwardIterator requirements requires dereference to return a reference.
         * Which cannot be as we create the entry on demand.
         *
         * So we are stick with declaring ourselves at `input_iterator`.
         */
        using iterator_category = std::input_iterator_tag;
        using value_type = Entry;
        using pointer = Entry*;
        using reference = Entry&;
        SearchIterator();
        SearchIterator(const SearchIterator& it);
        SearchIterator& operator=(const SearchIterator& it);
        SearchIterator(SearchIterator&& it);
        SearchIterator& operator=(SearchIterator&& it);
        ~SearchIterator();

        bool operator== (const SearchIterator& it) const;
        bool operator!= (const SearchIterator& it) const;

        SearchIterator& operator++();
        SearchIterator operator++(int);
        SearchIterator& operator--();
        SearchIterator operator--(int);

        std::string getPath() const;
        std::string getTitle() const;
        int getScore() const;
        std::string getSnippet() const;
        int getWordCount() const;
        DEPRECATED int getSize() const;
        int getFileIndex() const;
        Uuid getZimId() const;
        reference operator*() const;
        pointer operator->() const;

#ifdef ZIM_PRIVATE
        std::string getDbData() const;
#endif

    private:
        struct InternalData;
        std::unique_ptr<InternalData> internal;
        SearchIterator(InternalData* internal_data);

        bool isEnd() const;
};

} // namespace zim

#endif // ZIM_SEARCH_ITERATOR_H
