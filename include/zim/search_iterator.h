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

#ifndef ZIM_SEARCH_ITERATOR_H
#define ZIM_SEARCH_ITERATOR_H

#include <memory>
#include <iterator>
#include "entry.h"
#include "uuid.h"

namespace zim
{
class SearchResultSet;
class SearchIterator : public std::iterator<std::bidirectional_iterator_tag, Entry>
{
    friend class zim::SearchResultSet;
    public:
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
        int getSize() const;
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
