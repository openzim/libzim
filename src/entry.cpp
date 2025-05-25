/*
 * Copyright (C) 2021 Renaud Gaudin <rgaudin@gmail.com>
 * Copyright (C) 2020 Matthieu Gautier <mgautier@kymeria.fr>
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

#include <zim/entry.h>
#include <zim/error.h>
#include <zim/item.h>
#include <zim/tools.h>
#include "fileimpl.h"
#include "log.h"

log_define("zim.entry")

using namespace zim;

Entry::Entry(std::shared_ptr<const FileImpl> file, entry_index_type idx)
  : m_file(file),
    m_idx(idx),
    m_dirent(file->getDirent(entry_index_t(idx)))
{}

std::string Entry::getTitle() const
{
  return m_dirent->getTitle();
}

std::string Entry::getPath() const
{
  if (m_file->hasNewNamespaceScheme()) {
    return m_dirent->getPath();
  } else {
    return m_dirent->getLongPath();
  }
}

bool Entry::isRedirect() const
{
  return m_dirent->isRedirect();
}

Item Entry::getItem(bool follow) const
{
  if (isRedirect())
  {
    if (!follow)
      throw InvalidType(Formatter()
                        << "Entry " << getPath() << " is a redirect entry.");
    return getRedirect();
  }

  return Item(*this);
}

Item Entry::getRedirect() const {
  auto nextEntry = getRedirectEntry();
  auto watchdog = 50U;
  while (nextEntry.isRedirect() && --watchdog) {
    nextEntry = nextEntry.getRedirectEntry();
  }
  return nextEntry.getItem(false);
}

entry_index_type Entry::getRedirectEntryIndex() const  {
  if (!isRedirect())
    throw InvalidType(Formatter()
                      << "Entry " << getPath() << " is not a redirect entry.");

  return m_dirent->getRedirectIndex().v;
}

Entry Entry::getRedirectEntry() const  {
  return Entry(m_file, getRedirectEntryIndex());
}
