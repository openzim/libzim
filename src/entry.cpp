/*
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
#include "_dirent.h"
#include "fileimpl.h"
#include "file_part.h"
#include "log.h"

#include <sstream>

log_define("zim.entry")

using namespace zim;

Entry::Entry(std::shared_ptr<FileImpl> file, entry_index_type idx)
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
    return m_dirent->getUrl();
  } else {
    return m_dirent->getLongUrl();
  }
}

bool Entry::isRedirect() const
{
  return m_dirent->isRedirect();
}

Item Entry::getItem(bool follow) const
{
  if (isRedirect()) {
    if (! follow) {
      std::ostringstream sstream;
      sstream << "Entry " << m_idx << " is a redirect entry.";
      throw InvalidType(sstream.str());
    }
    return getRedirect();
 }

  return Item(m_file, m_idx);
}

Item Entry::getRedirect() const {
  auto nextEntry = getRedirectEntry();
  auto watchdog = 50U;
  while (nextEntry.isRedirect() && --watchdog) {
    nextEntry = nextEntry.getRedirectEntry();
  }
  return nextEntry.getItem(false);
}

Entry Entry::getRedirectEntry() const  {
  if (!isRedirect()) {
    std::ostringstream sstream;
    sstream << "Entry " << m_idx << " is not a redirect entry.";
    throw InvalidType(sstream.str());
  }
  return Entry(m_file, static_cast<entry_index_type>(m_dirent->getRedirectIndex()));
}
