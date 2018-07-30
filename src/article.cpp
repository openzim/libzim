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

#include <zim/article.h>
#include "template.h"
#include "_dirent.h"
#include "cluster.h"
#include <zim/fileheader.h>
#include "fileimpl.h"
#include "file_part.h"
#include <sstream>
#include <iostream>
#include <stdexcept>
#include "log.h"

log_define("zim.article")

namespace zim
{
  size_type Article::getArticleSize() const
  {
    auto dirent = getDirent();
    return size_type(file->getCluster(dirent->getClusterNumber())
                         ->getBlobSize(dirent->getBlobNumber()));
  }

  namespace
  {
    class Ev : public TemplateParser::Event
    {
        std::ostream& out;
        Article& article;
        std::shared_ptr<FileImpl> file;
        unsigned maxRecurse;

      public:
        Ev(std::ostream& out_, Article& article_, std::shared_ptr<FileImpl> file_, unsigned maxRecurse_)
          : out(out_),
            article(article_),
            file(file_),
            maxRecurse(maxRecurse_)
          { }
        void onData(const std::string& data);
        void onToken(const std::string& token);
        void onLink(char ns, const std::string& title);
    };

    void Ev::onData(const std::string& data)
    {
      out << data;
    }

    void Ev::onToken(const std::string& token)
    {
      log_trace("onToken(\"" << token << "\")");

      if (token == "title")
        out << article.getTitle();
      else if (token == "url")
        out << article.getUrl();
      else if (token == "namespace")
        out << article.getNamespace();
      else if (token == "content")
      {
        if (maxRecurse <= 0)
          throw std::runtime_error("maximum recursive limit is reached");
        article.getPage(out, false, maxRecurse - 1);
      }
      else
      {
        log_warn("unknown token \"" << token  << "\" found in template");
        out << "<%" << token << "%>";
      }
    }

    void Ev::onLink(char ns, const std::string& url)
    {
      if (maxRecurse <= 0)
        throw std::runtime_error("maximum recursive limit is reached");
      std::pair<bool, article_index_t> r = file->findx(ns, url);
      if (r.first) {
          Article(file, article_index_type(r.second)).getPage(out, false, maxRecurse - 1);
      } else {
          throw std::runtime_error(std::string("impossible to find article ") + std::string(1, ns) + std::string("/") + url);
      }
    }

  }

  std::shared_ptr<const Dirent> Article::getDirent() const
  {
    return file->getDirent(article_index_t(idx));
  }

  std::string Article::getParameter() const
  {
    return getDirent()->getParameter();
  }

  std::string Article::getTitle() const
  {
    return getDirent()->getTitle();
  }

  std::string Article::getUrl() const
  {
    return getDirent()->getUrl();
  }

  std::string Article::getLongUrl() const
  {
    return getDirent()->getLongUrl();
  }

  uint16_t Article::getLibraryMimeType() const
  {
    return getDirent()->getMimeType();
  }

  const std::string& Article::getMimeType() const
  {
    return file->getMimeType(getLibraryMimeType());
  }

  bool Article::isRedirect() const
  {
    return getDirent()->isRedirect();
  }

  bool Article::isLinktarget() const
  {
    return getDirent()->isLinktarget();
  }

  bool Article::isDeleted() const
  {
    return getDirent()->isDeleted();
  }

  char Article::getNamespace() const
  {
    return getDirent()->getNamespace();
  }

  article_index_type Article::getRedirectIndex() const
  {
    return article_index_type(getDirent()->getRedirectIndex());
  }

  Article Article::getRedirectArticle() const
  {
    return Article(file, getRedirectIndex());
  }

  std::shared_ptr<const Cluster> Article::getCluster() const
  {
    auto dirent = getDirent();
    if ( dirent->isRedirect()
      || dirent->isLinktarget()
      || dirent->isDeleted() ) {
      return std::shared_ptr<const Cluster>();
    }
    return file->getCluster(dirent->getClusterNumber());
  }

  Blob Article::getData(offset_type offset) const
  {
    auto size = getArticleSize()-offset;
    return getData(offset, size);
  }

  Blob Article::getData(offset_type offset, size_type size) const
  {
    std::shared_ptr<const Cluster> cluster = getCluster();
    if (!cluster) {
      return Blob();
    }
    return cluster->getBlob(getDirent()->getBlobNumber(), offset_t(offset), zsize_t(size));
  }

  offset_type Article::getOffset() const
  {
    auto dirent = getDirent();
    if (dirent->isRedirect()
        || dirent->isLinktarget()
        || dirent->isDeleted())
        return 0;
    return offset_type(file->getBlobOffset(dirent->getClusterNumber(), dirent->getBlobNumber()));
  }

  std::pair<std::string, offset_type> Article::getDirectAccessInformation() const
  {
    auto dirent = getDirent();
    if ( dirent->isRedirect()
      || dirent->isLinktarget()
      || dirent->isDeleted() ) {
        return std::make_pair("", 0);
    }

    auto full_offset = file->getBlobOffset(dirent->getClusterNumber(),
                                           dirent->getBlobNumber());

    if (!full_offset) {
      // cluster is compressed
      return std::make_pair("", 0);
    }
    auto part_its = file->getFileParts(full_offset, zsize_t(getArticleSize()));
    auto range = part_its.first->first;
    auto part = part_its.first->second;
    if (++part_its.first != part_its.second) {
      return std::make_pair("", 0);
    }
    auto local_offset = full_offset - range.min;
    return std::make_pair(part->filename(), offset_type(local_offset));

  }

  std::string Article::getPage(bool layout, unsigned maxRecurse)
  {
    std::ostringstream s;
    getPage(s, layout, maxRecurse);
    return s.str();
  }

  void Article::getPage(std::ostream& out, bool layout, unsigned maxRecurse)
  {
    log_trace("Article::getPage(" << layout << ", " << maxRecurse << ')');

    if (getMimeType().compare(0, 9, "text/html") == 0 || getMimeType() == MimeHtmlTemplate)
    {
      if (layout && file->getFileheader().hasLayoutPage())
      {
        Article layoutPage(file, file->getFileheader().getLayoutPage());
        Blob data = layoutPage.getData();

        Ev ev(out, *this, file, maxRecurse);
        log_debug("call template parser");
        TemplateParser parser(&ev);
        for (const char* p = data.data(); p != data.end(); ++p)
          parser.parse(*p);
        parser.flush();

        return;
      }
      else if (getMimeType() == MimeHtmlTemplate)
      {
        Blob data = getData();

        Ev ev(out, *this, file, maxRecurse);
        TemplateParser parser(&ev);
        for (const char* p = data.data(); p != data.end(); ++p)
          parser.parse(*p);
        parser.flush();

        return;
      }
    }

    // default case - template cases has return above
    out << getData();
  }

}
