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

#ifndef ZIM_DIRENT_H
#define ZIM_DIRENT_H

#include <string>
#include <zim/zim.h>
#include <exception>
#include <memory>

#include "zim_types.h"
#include "debug.h"

namespace zim
{
  class Buffer;
  class InvalidSize : public std::exception {};
  class Dirent
  {
    protected:
      uint16_t mimeType;

      uint32_t version;

      cluster_index_t clusterNumber;  // only used when redirect is false
      blob_index_t blobNumber;    // only used when redirect is false

      article_index_t redirectIndex;  // only used when redirect is true

      char ns;
      std::string title;
      std::string url;
      std::string parameter;

    public:
      // these constants are put into mimeType field
      static const uint16_t redirectMimeType = 0xffff;
      static const uint16_t linktargetMimeType = 0xfffe;
      static const uint16_t deletedMimeType = 0xfffd;

      Dirent()
        : mimeType(0),
          version(0),
          clusterNumber(0),
          blobNumber(0),
          redirectIndex(0),
          ns('\0')
      {}

      Dirent(std::unique_ptr<Buffer> buffer);

      bool isRedirect() const                 { return mimeType == redirectMimeType; }
      bool isLinktarget() const               { return mimeType == linktargetMimeType; }
      bool isDeleted() const                  { return mimeType == deletedMimeType; }
      bool isArticle() const                  { return !isRedirect() && !isLinktarget() && !isDeleted(); }
      uint16_t getMimeType() const            { return mimeType; }

      uint32_t getVersion() const            { return version; }
      void setVersion(uint32_t v)            { version = v; }

      cluster_index_t getClusterNumber() const      { return isRedirect() ? cluster_index_t(0) : clusterNumber; }
      blob_index_t  getBlobNumber() const         { return isRedirect() ? blob_index_t(0) : blobNumber; }

      article_index_t getRedirectIndex() const      { return isRedirect() ? redirectIndex : article_index_t(0); }

      char getNamespace() const               { return ns; }
      const std::string& getTitle() const     { return title.empty() ? url : title; }
      const std::string& getUrl() const       { return url; }
      std::string getLongUrl() const;
      const std::string& getParameter() const { return parameter; }

      size_t getDirentSize() const
      {
        size_t ret = (isRedirect() ? 12 : 16) + url.size() + parameter.size() + 2;
        if (title != url)
          ret += title.size();
        return ret;
      }

      void setTitle(const std::string& title_)
      {
        title = title_;
      }

      void setUrl(char ns_, const std::string& url_)
      {
        ns = ns_;
        url = url_;
      }

      void setParameter(const std::string& parameter_)
      {
        parameter = parameter_;
      }

      void setRedirect(article_index_t idx)
      {
        redirectIndex = idx;
        mimeType = redirectMimeType;
      }

      void setMimeType(uint16_t mime)
      {
        mimeType = mime;
      }

      void setLinktarget()
      {
        ASSERT(mimeType, ==, 0);
        mimeType = linktargetMimeType;
      }

      void setDeleted()
      {
        ASSERT(mimeType, ==, 0);
        mimeType = deletedMimeType;
      }

      void setArticle(uint16_t mimeType_, cluster_index_t clusterNumber_, blob_index_t blobNumber_)
      {
        ASSERT(mimeType, ==, 0);
        mimeType = mimeType_;
        clusterNumber = clusterNumber_;
        blobNumber = blobNumber_;
      }

  };

}

#endif // ZIM_DIRENT_H

