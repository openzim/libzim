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
        static const uint32_t version = 0;

        uint16_t mimeType;
        DirentInfo info {};
        char ns;
        std::string path;
        std::string title;
        Cluster* cluster = nullptr;
        char redirectNs;
        std::string redirectPath;
        entry_index_t idx = entry_index_t(0);
        offset_t offset;
        bool removed;

      public:
        Dirent()
          : mimeType(0),
            ns(),
            path(),
            title(),
            redirectNs(),
            redirectPath(),
            removed(false)
        {
          info.d.clusterNumber = cluster_index_t(0);
          info.d.blobNumber = blob_index_t(0);
        }

        explicit Dirent(char ns_, std::string path_ )
          : Dirent()
          { ns = ns_; path = path_; }

        char getNamespace() const                { return ns; }
        void setNamespace(char ns_)              { ns = ns_; }
        const std::string& getTitle() const      { return title.empty() ? path : title; }
        const std::string& getRealTitle() const  { return title; }
        void setTitle(const std::string& title_) { title = title_; }
        const std::string& getPath() const       { return path; }
        void setPath(const std::string& path_) {
          path = path_;
        }

        uint32_t getVersion() const            { return version; }

        void setRedirectNs(char redirectNs_)      { redirectNs = redirectNs_; }
        char getRedirectNs() { return redirectNs; }
        void setRedirectPath(const std::string& redirectPath_)     { redirectPath = redirectPath_; }
        const std::string& getRedirectPath() const         { return redirectPath; }
        void setRedirect(const Dirent* target) {
          info.r.redirectDirent = target;
          mimeType = redirectMimeType;
        }
        entry_index_t getRedirectIndex() const      { return isRedirect() ? info.r.redirectDirent->getIdx() : entry_index_t(0); }

        void setMimeType(uint16_t mime)
        {
          mimeType = mime;
        }

        void setIdx(entry_index_t idx_)      { idx = idx_; }
        entry_index_t getIdx() const         { return idx; }


        void setCluster(zim::writer::Cluster* _cluster)
        {
          ASSERT(isItem(), ==, true);
          cluster = _cluster;
          info.d.blobNumber = _cluster->count();
        }

        zim::writer::Cluster* getCluster()
        {
          return cluster;
        }

        cluster_index_t getClusterNumber() const {
          return cluster ? cluster->getClusterIndex() : info.d.clusterNumber;
        }
        blob_index_t  getBlobNumber() const {
          return isRedirect() ? blob_index_t(0) : info.d.blobNumber;
        }

        bool isRedirect() const                 { return mimeType == redirectMimeType; }
        bool isItem() const                     { return !isRedirect(); }
        uint16_t getMimeType() const            { return mimeType; }
        size_t getDirentSize() const
        {
          size_t ret = (isRedirect() ? 12 : 16) + path.size() + 2;
          if (title != path)
            ret += title.size();
          return ret;
        }

        offset_t getOffset() const { return offset; }
        void setOffset(offset_t o) { offset = o; }

        void setItem(uint16_t mimeType_, cluster_index_t clusterNumber_, blob_index_t blobNumber_)
        {
          ASSERT(mimeType, ==, 0);
          mimeType = mimeType_;
          info.d.clusterNumber = clusterNumber_;
          info.d.blobNumber = blobNumber_;
        }

        bool isRemoved() const { return removed; }
        void markRemoved() { removed = true; }

        void write(int out_fd) const;

        friend bool compareUrl(const Dirent* d1, const Dirent* d2);
        friend inline bool compareTitle(const Dirent* d1, const Dirent* d2);
    };


    inline bool compareUrl(const Dirent* d1, const Dirent* d2)
    {
      return d1->ns < d2->ns || (d1->ns == d2->ns && d1->path < d2->path);
    }
    inline bool compareTitle(const Dirent* d1, const Dirent* d2)
    {
      return d1->ns < d2->ns
        || (d1->ns == d2->ns && d1->getTitle() < d2->getTitle());
    }
  }
}

#endif // ZIM_WRITER_DIRENT_H

