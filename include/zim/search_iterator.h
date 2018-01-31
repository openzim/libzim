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
#include "article.h"

namespace zim
{
class Search;
class search_iterator : public std::iterator<std::bidirectional_iterator_tag, Article>
{
    friend class zim::Search;
    public:
        search_iterator();
        search_iterator(const search_iterator& it);
        search_iterator& operator=(const search_iterator& it);
        search_iterator(search_iterator&& it);
        search_iterator& operator=(search_iterator&& it);
        ~search_iterator();

        bool operator== (const search_iterator& it) const;
        bool operator!= (const search_iterator& it) const;

        search_iterator& operator++();
        search_iterator operator++(int);
        search_iterator& operator--();
        search_iterator operator--(int);

        std::string get_url() const;
        std::string get_title() const;
        int get_score() const;
        std::string get_snippet() const;
        int get_wordCount() const;
        int get_size() const;
        int get_fileIndex() const;
        reference operator*() const;
        pointer operator->() const;

    private:
        struct InternalData;
        std::unique_ptr<InternalData> internal;
        search_iterator(InternalData* internal_data);

        bool is_end() const;
};

} // namespace ziç

#endif // ZIM_SEARCH_ITERATOR_H
