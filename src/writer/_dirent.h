/*
 * Copyright (C) 2009 Tommi Maekitalo
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

#ifndef ZIM_WRITER_DIRENT_H
#define ZIM_WRITER_DIRENT_H

#include "cluster.h"

#include "debug.h"

namespace zim
{
  namespace writer {
    class Dirent;
    struct DirectInfo {
      DirectInfo() :
        clusterNumber(0),
        blobNumber(0)
      {};
      cluster_index_t  clusterNumber;
      blob_index_t     blobNumber;
    };

    struct RedirectInfo {
      const Dirent* redirectDirent = nullptr;
    };

    union DirentInfo {
      DirectInfo d;
      RedirectInfo r;
    };

    class Dirent
    {
        static const uint16_t redirectMimeType = 0xffff;
        static const uint16_t linktargetMimeType = 0xfffe;
        static const uint16_t deletedMimeType = 0xfffd;
        static const uint32_t version = 0;

        uint16_t mimeType;
        DirentInfo info {};
        Url url;
        std::string title;
        Cluster* cluster = nullptr;
        Url redirectUrl;
        article_index_t idx = article_index_t(0);

      public:
        Dirent()
          : mimeType(0),
            url(),
            title(),
            redirectUrl()
        {
          info.d.clusterNumber = cluster_index_t(0);
          info.d.blobNumber = blob_index_t(0);
        }

        explicit Dirent(Url url_ )
          : Dirent()
          { url = url_; }

        char getNamespace() const               { return url.getNs(); }
        const std::string& getTitle() const     { return title.empty() ? url.getUrl() : title; }
        void setTitle(const std::string& title_) { title = title_; }
        const std::string& getUrl() const       { return url.getUrl(); }
        const Url& getFullUrl() const { return url; }
        void setUrl(Url url_) {
          url = url_;
        }

        uint32_t getVersion() const            { return version; }

        void setRedirectUrl(Url redirectUrl_)     { redirectUrl = redirectUrl_; }
        const Url& getRedirectUrl() const         { return redirectUrl; }
        void setRedirect(const Dirent* target) {
          info.r.redirectDirent = target;
          mimeType = redirectMimeType;
        }
        article_index_t getRedirectIndex() const      { return isRedirect() ? info.r.redirectDirent->getIdx() : article_index_t(0); }

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


        void setIdx(article_index_t idx_)      { idx = idx_; }
        article_index_t getIdx() const         { return idx; }


        void setCluster(zim::writer::Cluster* _cluster)
        {
          ASSERT(isArticle(), ==, true);
          cluster = _cluster;
          info.d.blobNumber = _cluster->count();
        }

        cluster_index_t getClusterNumber() const {
          return cluster ? cluster->getClusterIndex() : info.d.clusterNumber;
        }
        blob_index_t  getBlobNumber() const {
          return isRedirect() ? blob_index_t(0) : info.d.blobNumber;
        }

        bool isRedirect() const                 { return mimeType == redirectMimeType; }
        bool isLinktarget() const               { return mimeType == linktargetMimeType; }
        bool isDeleted() const                  { return mimeType == deletedMimeType; }
        bool isArticle() const                  { return !isRedirect() && !isLinktarget() && !isDeleted(); }
        uint16_t getMimeType() const            { return mimeType; }
        size_t getDirentSize() const
        {
          size_t ret = (isRedirect() ? 12 : 16) + url.getUrl().size() + 2;
          if (title != url.getUrl())
            ret += title.size();
          return ret;
        }

        void setArticle(uint16_t mimeType_, cluster_index_t clusterNumber_, blob_index_t blobNumber_)
        {
          ASSERT(mimeType, ==, 0);
          mimeType = mimeType_;
          info.d.clusterNumber = clusterNumber_;
          info.d.blobNumber = blobNumber_;
        }



        friend bool compareUrl(const Dirent* d1, const Dirent* d2);
        friend inline bool compareTitle(const Dirent* d1, const Dirent* d2);
    };


    inline bool compareUrl(const Dirent* d1, const Dirent* d2)
    {
      return d1->url < d2->url;
    }
    inline bool compareTitle(const Dirent* d1, const Dirent* d2)
    {
      return d1->url.getNs() < d2->url.getNs()
        || (d1->url.getNs() == d2->url.getNs() && d1->getTitle() < d2->getTitle());
    }

    std::ostream& operator<< (std::ostream& out, const Dirent& d);

  }
}

#endif // ZIM_WRITER_DIRENT_H

