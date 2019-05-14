/*
 * Copyright (C) 2006,2009 Tommi Maekitalo
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

#include <zim/file.h>
#include "fileimpl.h"
#include <zim/article.h>
#include <zim/search.h>
#include "log.h"
#include <zim/fileiterator.h>
#include <zim/error.h>

log_define("zim.file")

namespace zim
{
  namespace
  {
    int hexval(char ch)
    {
      if (ch >= '0' && ch <= '9')
        return ch - '0';
      if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
      if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
      return -1;
    }
  }

  File::File(const std::string& fname)
    : impl(new FileImpl(fname))
    { }

  const std::string& File::getFilename() const
  {
    return impl->getFilename();
  }

  const Fileheader& File::getFileheader() const
  {
    return impl->getFileheader();
  }

  size_type File::getFilesize() const
  {
    return impl->getFilesize().v;
  }

  article_index_type File::getCountArticles() const
  {
    return article_index_type(impl->getCountArticles());
  }


  Article File::getArticle(article_index_type idx) const
  {
    if (idx >= article_index_type(impl->getCountArticles()))
      throw ZimFileFormatError("article index out of range");
    return Article(impl, idx);
  }

  Article File::getArticle(char ns, const std::string& url) const
  {
    log_trace("File::getArticle('" << ns << "', \"" << url << ')');
    std::pair<bool, article_index_t> r = impl->findx(ns, url);
    return r.first ? Article(impl, article_index_type(r.second)) : Article();
  }

  Article File::getArticleByUrl(const std::string& url) const
  {
    log_trace("File::getArticle(\"" << url << ')');
    std::pair<bool, article_index_t> r = impl->findx(url);
    return r.first ? Article(impl, article_index_type(r.second)) : Article();
  }

  Article File::getArticleByTitle(article_index_type idx) const
  {
    return Article(impl, article_index_type(impl->getIndexByTitle(article_index_t(idx))));
  }

  Article File::getArticleByTitle(char ns, const std::string& title) const
  {
    log_trace("File::getArticleByTitle('" << ns << "', \"" << title << ')');
    std::pair<bool, article_index_t> r = impl->findxByTitle(ns, title);
    return r.first
            ? Article(impl, article_index_type(impl->getIndexByTitle(r.second)))
            : Article();
  }

  std::shared_ptr<const Cluster> File::getCluster(cluster_index_type idx) const
  {
    return impl->getCluster(cluster_index_t(idx));
  }

  cluster_index_type File::getCountClusters() const
  {
    return cluster_index_type(impl->getCountClusters());
  }

  offset_type File::getClusterOffset(cluster_index_type idx) const
  {
    return offset_type(impl->getClusterOffset(cluster_index_t(idx)));
  }

  Blob File::getBlob(cluster_index_type clusterIdx, blob_index_type blobIdx) const
  {
    return impl->getCluster(cluster_index_t(clusterIdx))->getBlob(blob_index_t(blobIdx));
  }

  article_index_type File::getNamespaceBeginOffset(char ch) const
  {
    return article_index_type(impl->getNamespaceBeginOffset(ch));
  }

  article_index_type File::getNamespaceEndOffset(char ch) const
  {
    return article_index_type(impl->getNamespaceEndOffset(ch));
  }

  article_index_type File::getNamespaceCount(char ns) const
  {
    return getNamespaceEndOffset(ns) - getNamespaceBeginOffset(ns);
  }

  std::string File::getNamespaces() const
  {
    return impl->getNamespaces();
  }

  bool File::hasNamespace(char ch) const
  {
    article_index_t off = impl->getNamespaceBeginOffset(ch);
    return off < impl->getCountArticles() && impl->getDirent(off)->getNamespace() == ch;
  }

  File::const_iterator File::begin() const
  { return const_iterator(this, 0); }

  File::const_iterator File::beginByTitle() const
  { return const_iterator(this, 0, const_iterator::ArticleIterator); }

  File::const_iterator File::end() const
  { return const_iterator(this, getCountArticles()); }

  File::const_iterator File::find(char ns, const std::string& url) const
  {
    std::pair<bool, article_index_t> r = impl->findx(ns, url);
    return File::const_iterator(this, article_index_type(r.second));
  }

  File::const_iterator File::find(const std::string& url) const
  {
    std::pair<bool, article_index_t> r = impl->findx(url);
    return File::const_iterator(this, article_index_type(r.second));
  }

  File::const_iterator File::findByTitle(char ns, const std::string& title) const
  {
    std::pair<bool, article_index_t> r = impl->findxByTitle(ns, title);
    return File::const_iterator(this, article_index_type(r.second), const_iterator::ArticleIterator);
  }

  std::unique_ptr<Search> File::search(const std::string& query, int start, int end) const {
      auto search = std::unique_ptr<Search>(new Search(this));
      search->set_query(query);
      search->set_range(start, end);
      return search;
  }

  std::unique_ptr<Search> File::suggestions(const std::string& query, int start, int end) const {
      auto search = std::unique_ptr<Search>(new Search(this));
      search->set_query(query);
      search->set_range(start, end);
      search->set_suggestion_mode(true);
      return search;
  }

  offset_type File::getOffset(cluster_index_type clusterIdx, blob_index_type blobIdx) const
  {
    return offset_type(impl->getBlobOffset(
                           cluster_index_t(clusterIdx),
                           blob_index_t(blobIdx)));
  }

  time_t File::getMTime() const
  {
    return impl->getMTime();
  }

  const std::string& File::getMimeType(uint16_t idx) const
  {
    return impl->getMimeType(idx);
  }

  std::string File::getChecksum()
  {
    return impl->getChecksum();
  }

  bool File::verify()
  {
    return impl->verify();
  }

  bool File::is_multiPart() const
  {
    return impl->is_multiPart();
  }


  std::string urldecode(const std::string& url)
  {
    std::string ret;
    enum {
      state_0,
      state_h1,
      state_h2
    } state = state_0;

    char ch = '\0';
    for (std::string::const_iterator it = url.begin(); it != url.end(); ++it)
    {
      switch (state)
      {
        case state_0:
          if (*it == '+')
            ret += ' ';
          else if (*it == '%')
            state = state_h1;
          else
            ret += *it;
          break;

        case state_h1:
          if ( (*it >= '0' && *it <= '9')
            || (*it >= 'A' && *it <= 'F')
            || (*it >= 'a' && *it <= 'f'))
          {
            ch = *it;
            state = state_h2;
          }
          else
          {
            ret += '%';
            ret += *it;
            state = state_0;
          }
          break;

        case state_h2:
          if ( (*it >= '0' && *it <= '9')
            || (*it >= 'A' && *it <= 'F')
            || (*it >= 'a' && *it <= 'f'))
          {
            ret += static_cast<char>(hexval(ch) * 16 + hexval(*it));
          }
          else
          {
            ret += static_cast<char>(hexval(ch));
            ret += *it;
          }
          state = state_0;
          break;
      }

    }

    switch (state)
    {
      case state_0:
        break;

      case state_h1:
        ret += '%';
        break;

      case state_h2:
        ret += '%';
        ret += ch;
        break;
    }

    return ret;
  }
}
