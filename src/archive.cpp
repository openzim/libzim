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
#include "tools.h"
#include "log.h"

log_define("zim.archive")

namespace zim
{
  Archive::Archive(const std::string& fname)
    : m_impl(new FileImpl(fname))
    { }

  Archive::Archive(int fd)
    : m_impl(new FileImpl(fd))
    { }

  const std::string& Archive::getFilename() const
  {
    return m_impl->getFilename();
  }

  size_type Archive::getFilesize() const
  {
    return m_impl->getFilesize().v;
  }

  entry_index_type Archive::getEntryCount() const
  {
    return m_impl->getUserEntryCount().v;
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
    return item.getData();
  }

  std::vector<std::string> Archive::getMetadataKeys() const {
    std::vector<std::string> ret;
    auto start = m_impl->getNamespaceBeginOffset('M');
    auto end = m_impl->getNamespaceEndOffset('M');
    for (auto idx=start; idx!=end; idx++) {
      auto dirent = m_impl->getDirent(idx);
      ret.push_back(dirent->getUrl());
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
    if (m_impl->hasNewNamespaceScheme()) {
      // Get path in user content.
      auto r = m_impl->findx('C', path);
      if (r.first) {
        return Entry(m_impl, entry_index_type(r.second));
      }
      try {
        // Path may come from a already stored from a old zim archive (bookmark),
        // and so contains a namespace.
        // We have to adapt the path to use the C namespace.
        r = m_impl->findx('C', std::get<1>(parseLongPath(path)));
        if (r.first) {
          return Entry(m_impl, entry_index_type(r.second));
        }
      } catch (std::runtime_error&) {}
    } else {
      // Path should contains the namespace.
      auto r = m_impl->findx(path);
      if (r.first) {
        return Entry(m_impl, entry_index_type(r.second));
      }
      // If not (bookmark) from a recent zim archive.
      for (auto ns:{'A', 'I', 'J', '-'}) {
        r = m_impl->findx(ns, path);
        if (r.first) {
          return Entry(m_impl, entry_index_type(r.second));
        }
      }
    }

    throw EntryNotFound("Cannot find entry");
  }

  Entry Archive::getEntryByTitle(entry_index_type idx) const
  {
    return Entry(m_impl, entry_index_type(m_impl->getIndexByTitle(title_index_t(idx))));
  }

  Entry Archive::getEntryByTitle(const std::string& title) const
  {
    for (auto ns:{'C', 'A', 'I', 'J', '-'}) {
      log_trace("File::getArticleByTitle('" << ns << "', \"" << title << ')');
      auto r = m_impl->findxByTitle(ns, title);
      if (r.first)
        return getEntryByTitle(entry_index_type(r.second));
    }
    throw EntryNotFound("Cannot find entry");
  }

  Entry Archive::getEntryByClusterOrder(entry_index_type idx) const
  {
     return Entry(m_impl, entry_index_type(m_impl->getIndexByClusterOrder(entry_index_t(idx))));
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

  Entry Archive::getFaviconEntry() const {
    // `-/favicon` is the standard path for the favicon, but older zims may have it
    // on other path.
    for(auto ns:{'-', 'I'}) {
      for (auto& path:{"favicon", "favicon.png"}) {
        auto r = m_impl->findx(ns, path);
        if (r.first) {
          return getEntryByPath(entry_index_type(r.second));
        }
      }
    }
    throw EntryNotFound("Cannot find favicon entry");
  }

  bool Archive::hasFaviconEntry() const {
    try {
      getFaviconEntry();
      return true;
    } catch (EntryNotFound& e) {
      return false;
    }
  }

  bool Archive::hasFulltextIndex() const {
    auto r = m_impl->findx('X', "fulltext/xapian");
    if (!r.first) {
      r = m_impl->findx('Z', "/fulltextIndex/xapian");
    }
    if (!r.first) {
      return false;
    }
    auto entry = Entry(m_impl, entry_index_type(r.second));
    auto item = entry.getItem(true);
    auto accessInfo = item.getDirectAccessInformation();
    return accessInfo.second;
  }

  bool Archive::hasTitleIndex() const {
    auto r = m_impl->findx('X', "title/xapian");
    if (!r.first) {
      return false;
    }
    auto entry = Entry(m_impl, entry_index_type(r.second));
    auto item = entry.getItem(true);
    auto accessInfo = item.getDirectAccessInformation();
    return accessInfo.second;
  }

  Archive::EntryRange<EntryOrder::pathOrder> Archive::iterByPath() const
  {
    return EntryRange<EntryOrder::pathOrder>(m_impl, m_impl->getStartUserEntry().v, m_impl->getEndUserEntry().v);
  }

  Archive::EntryRange<EntryOrder::titleOrder> Archive::iterByTitle() const
  {
    return EntryRange<EntryOrder::titleOrder>(m_impl, m_impl->getStartUserEntry().v, m_impl->getEndUserEntry().v);
  }

  Archive::EntryRange<EntryOrder::efficientOrder> Archive::iterEfficient() const
  {
    return EntryRange<EntryOrder::efficientOrder>(m_impl, 0, getEntryCount());
  }

  Archive::EntryRange<EntryOrder::pathOrder> Archive::findByPath(std::string path) const
  {
    // "url order" means that the entries are stored by long url ("NS/url)".
    //
    // If we really want to search by url whatever is the namespace, we would have to
    // search in all "content" (A, I, J, -) namespaces and then merge the results.
    //
    // It would be pretty complex as we would need to have iterate hover several ranges
    // in the same time. Let's enforce that path is the full path and search in whatever
    // namespace is in it.

    // We have to return two iterator for a range of entry where `path` is a prefix.
    // - The begin iterator is a iterator to the first entry with `path`  as a prefix (or (range) end if none)
    // - The end iterator is the iterator pass the last entry with `path` as a prefix (or (gloabal) end)
    //
    // The findx return a iterator for the exact match or the one just after.
    // So, for the begin iterator, we can simply use the index returned by findx
    // For the end iterator we have to do the same but with a prefix "just after" the queried `path`
    // So the end index will always be just after the prefix range. If there is no prefix range, both
    // begin and end will be just after where it would be.
    //
    // Suposing a list of title :
    // 0. aaaaaa
    // 1. aaaaab
    // 2. aabbaa
    // 3. aabbbb
    // 4. bbaaaa
    // 5. bbbb
    // 6. bbbbaa
    // 7. bbbbbb
    // 8. <past the end>

    // If we search for prefix aabb, we must return 2/4
    // A findx with aabb will return 2
    // A findx with aabc will return 4
    //
    // If we search for prefix bbbb, we must return 5/8
    // A findx with bbbb will return 5 (with exact match)
    // A findx with bbbc will return 8
    //
    // If we search for prefix cccc, we must return 8/8
    // A findx with cccc will return 8
    // A findx with bbbc will return 8
    //
    // If we search for prefix a, we must return 0/4
    // A findx with a will return 0
    // A find with b will return 4
    entry_index_t begin_idx, end_idx;
    if (m_impl->hasNewNamespaceScheme()) {
      begin_idx = m_impl->findx('C', path).second;
      path.back()++;
      end_idx = m_impl->findx('C', path).second;
    } else {
      begin_idx = m_impl->findx(path).second;
      path.back()++;
      end_idx = m_impl->findx(path).second;
    }
    return Archive::EntryRange<EntryOrder::pathOrder>(m_impl, begin_idx.v, end_idx.v);
  }

  Archive::EntryRange<EntryOrder::titleOrder> Archive::findByTitle(std::string title) const
  {
    // "title order" means that the entries are stored by "NS/title" part.
    // It is nice when we want to search for title in a specific namespace, but
    // now we want to hide the namespace. It would be better if the "title order"
    // would be real title order, whatever is the namespace.
    //
    // If we really want to search by title what ever is the namespace, we would have to
    // search in all "content" namespace and then merge the results.
    //
    // The find by title is only used for the article (`A` namespace). So let's search
    // only in it.

    // See `Archive::findByPath` for the rational.
    auto ns = m_impl->hasNewNamespaceScheme() ? 'C' : 'A';
    auto begin_idx = m_impl->findxByTitle(ns, title).second;
    title.back()++;
    auto end_idx = m_impl->findxByTitle(ns, title).second;
    return Archive::EntryRange<EntryOrder::titleOrder>(m_impl, begin_idx.v, end_idx.v);
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

  bool Archive::hasNewNamespaceScheme() const
  {
    return m_impl->hasNewNamespaceScheme();
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
    return impl.getIndexByTitle(title_index_t(idx)).v;
  }

  template<>
  entry_index_type
  _toPathOrder<EntryOrder::efficientOrder>(const FileImpl& impl, entry_index_type idx)
  {
    return impl.getIndexByClusterOrder(entry_index_t(idx)).v;
  }

  bool Archive::checkIntegrity(IntegrityCheck checkType)
  {
    return m_impl->checkIntegrity(checkType);
  }

  bool validate(const std::string& zimPath, IntegrityCheckList checksToRun)
  {
    try
    {
      Archive a(zimPath);
      for ( size_t i = 0; i < checksToRun.size(); ++i )
      {
        if ( checksToRun.test(i) && !a.checkIntegrity(IntegrityCheck(i)) )
          return false;
      }
    }
    catch(ZimFileFormatError &exception)
    {
      std::cerr << exception.what() << std::endl;
      return false;
    }

    return true;
  }

} // namespace zim
