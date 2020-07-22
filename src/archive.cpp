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

#define ZIM_PRIVATE
#include <zim/archive.h>
#include <zim/entry.h>
#include <zim/item.h>
#include <zim/error.h>
#include "fileimpl.h"
#include "log.h"

log_define("zim.archive")

namespace zim
{
  Archive::Archive(const std::string& fname)
    : m_impl(new FileImpl(fname))
    { }

  const std::string& Archive::getFilename() const
  {
    return m_impl->getFilename();
  }

  size_type Archive::getFilesize() const
  {
    return m_impl->getFilesize().v;
  }

  article_index_type Archive::getEntryCount() const
  {
    return article_index_type(m_impl->getCountArticles());
  }

  Uuid Archive::getUuid() const
  {
    return m_impl->getFileheader().getUuid();
  }

  std::string Archive::getMetadata(const std::string& name) const
  {
    auto r = m_impl->findx('M', name);
    if (!r.first) {
      throw EntryNotFound("Cannot find metadata");
    }
    auto entry = Entry(m_impl, entry_index_type(r.second));
    auto item = entry.getItem(true);
    return item->getData();
  }

  std::vector<std::string> Archive::getMetadataKeys() const {
    std::vector<std::string> ret;
    auto start = entry_index_type(m_impl->getNamespaceBeginOffset('M'));
    auto end = entry_index_type(m_impl->getNamespaceEndOffset('M'));
    for (auto idx=start; idx!=end; idx++) {
      auto entry = Entry(m_impl, entry_index_type(idx));
      ret.push_back(entry.getPath());
    }
    return ret;
  }

  Entry Archive::getEntryByPath(entry_index_type idx) const
  {
    if (idx >= entry_index_type(m_impl->getCountArticles()))
      throw std::out_of_range("entry index out of range");
    return Entry(m_impl, idx);
  }

  Entry Archive::getEntryByPath(const std::string& path) const
  {
    log_trace("File::getArticle('" << path << ')');
    auto r = m_impl->findx(path);
    if (!r.first) {
      auto fallback_path = "A/" + path;
      r = m_impl->findx(fallback_path);
    }

    if (r.first) {
      return Entry(m_impl, entry_index_type(r.second));
    }
    throw EntryNotFound("Cannot find entry");
  }

  Entry Archive::getEntryByTitle(entry_index_type idx) const
  {
    return Entry(m_impl, entry_index_type(m_impl->getIndexByTitle(article_index_t(idx))));
  }

  Entry Archive::getEntryByTitle(const std::string& title) const
  {
    log_trace("File::getArticleByTitle('" << 'A' << "', \"" << title << ')');
    auto r = m_impl->findxByTitle('A', title);
    if (r.first)
      return getEntryByTitle(entry_index_type(r.second));
    throw EntryNotFound("Cannot find entry");
  }

  Entry Archive::getEntryByClusterOrder(article_index_type idx) const
  {
     return Entry(m_impl, entry_index_type(m_impl->getIndexByClusterOrder(article_index_t(idx))));
  }

  Entry Archive::getMainEntry() const {
    auto& header = m_impl->getFileheader();
    if (!header.hasMainPage()) {
      throw EntryNotFound("No main page");
    }
    return getEntryByPath(header.getMainPage());
  }

  bool Archive::hasMainEntry() const {
    return m_impl->getFileheader().hasMainPage();
  }

  Archive::iterator<EntryOrder::pathOrder> Archive::findByPath(const std::string& path) const
  {
    auto r = m_impl->findx(path);
    if (r.first) {
      return iterator<EntryOrder::pathOrder>(m_impl, r.second.v);
    }
    return end<EntryOrder::pathOrder>();
  }

  Archive::iterator<EntryOrder::titleOrder> Archive::findByTitle(const std::string& title) const
  {
    auto r = m_impl->findxByTitle('A', title);
    if (r.first) {
      return iterator<EntryOrder::titleOrder>(m_impl, entry_index_type(r.second));
    }
    return end<EntryOrder::titleOrder>();
  }

  bool Archive::hasChecksum() const
  {
    return m_impl->getFileheader().hasChecksum();
  }

  std::string Archive::getChecksum() const
  {
    return m_impl->getChecksum();
  }

  bool Archive::check() const
  {
    return m_impl->verify();
  }

  bool Archive::is_multiPart() const
  {
    return m_impl->is_multiPart();
  }

  cluster_index_type Archive::getClusterCount() const
  {
    return cluster_index_type(m_impl->getCountClusters());
  }

  offset_type Archive::getClusterOffset(cluster_index_type idx) const
  {
    return offset_type(m_impl->getClusterOffset(cluster_index_t(idx)));
  }

  entry_index_type Archive::getMainEntryIndex() const
  {
    return m_impl->getFileheader().getMainPage();
  }

  template<>
  entry_index_type
  _toPathOrder<EntryOrder::pathOrder>(const FileImpl& impl, entry_index_type idx)
  {
    return idx;
  }

  template<>
  entry_index_type
  _toPathOrder<EntryOrder::titleOrder>(const FileImpl& impl, entry_index_type idx)
  {
    return impl.getIndexByTitle(article_index_t(idx)).v;
  }

  template<>
  entry_index_type
  _toPathOrder<EntryOrder::efficientOrder>(const FileImpl& impl, entry_index_type idx)
  {
    return impl.getIndexByClusterOrder(article_index_t(idx)).v;
  }

}
