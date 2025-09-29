/*
 * Copyright (C) 2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#ifndef ZIM_SUGGESTION_INTERNAL_H
#define ZIM_SUGGESTION_INTERNAL_H

#include "zim/suggestion.h"
#include "zim/archive.h"

#include <stdexcept>
#include <mutex>

#if defined(LIBZIM_WITH_XAPIAN)
#include <xapian.h>
#endif

namespace zim
{

/**
 * A class to encapsulate a xapian title index and it's archive and all the
 * information we can gather from it.
 */
class SuggestionDataBase {
  public: // methods
    SuggestionDataBase(const Archive& archive, bool verbose);

  public: // data
    // The archive to get suggestions from.
    Archive m_archive;

    // Verbosity of operations.
    bool m_verbose;

  private: // data
    std::mutex m_mutex;

#if defined(LIBZIM_WITH_XAPIAN)

  public: // xapian based methods
    bool hasDatabase() const;
    bool hasValuesmap() const;
    bool hasValue(const std::string& valueName) const;
    int  valueSlot(const std::string&  valueName) const;

    Xapian::Query parseQuery(const std::string& query);

  public: // xapian based data
    // The Xapian database we will search on.
    Xapian::Database m_database;

    // The valuesmap associated with the database.
    std::map<std::string, int> m_valuesmap;

    // The query parser corresponding to the database.
    Xapian::QueryParser m_queryParser;

    // The stemmer used to parse queries
    Xapian::Stem m_stemmer;

  private:
    void initXapianDb();
#endif  // LIBZIM_WITH_XAPIAN
};


}

#endif // ZIM_SUGGESTION_INTERNAL_H
