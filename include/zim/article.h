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

#ifndef ZIM_ARTICLE_H
#define ZIM_ARTICLE_H

#include <string>
#include "zim.h"
#include "blob.h"
#include <limits>
#include <iosfwd>

#ifdef max
#undef max
#endif

namespace zim
{
  class Cluster;
  class Dirent;
  class FileImpl;

  class Article
  {
    private:
      std::shared_ptr<FileImpl> file;
      article_index_type idx;

      std::shared_ptr<const Dirent> getDirent() const;

    public:
      Article()
        : idx(std::numeric_limits<article_index_type>::max())
          { }

      Article(std::shared_ptr<FileImpl> file_, article_index_type idx_)
        : file(file_),
          idx(idx_)
          { }

      std::string getParameter() const;

      std::string getTitle() const;
      std::string getUrl() const;
      std::string getLongUrl() const;

      uint16_t    getLibraryMimeType() const;
      const std::string&  getMimeType() const;

      bool        isRedirect() const;
      bool        isLinktarget() const;
      bool        isDeleted() const;

      char        getNamespace() const;

      article_index_type   getRedirectIndex() const;
      Article     getRedirectArticle() const;

      size_type   getArticleSize() const;

      bool operator< (const Article& a) const
        { return getNamespace() < a.getNamespace()
              || (getNamespace() == a.getNamespace()
               && getTitle() < a.getTitle()); }

      std::shared_ptr<const Cluster> getCluster() const;

      Blob getData(offset_type offset=0) const;
      Blob getData(offset_type offset, size_type size) const;

      offset_type getOffset() const;
      std::pair<std::string, offset_type> getDirectAccessInformation() const;

      std::string getPage(bool layout = true, unsigned maxRecurse = 10);
      void getPage(std::ostream&, bool layout = true, unsigned maxRecurse = 10);

      article_index_type   getIndex() const   { return idx; }

      bool good() const   { return idx != std::numeric_limits<article_index_type>::max(); }
  };

}

#endif // ZIM_ARTICLE_H

