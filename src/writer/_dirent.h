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
#include "tinyString.h"

#include "debug.h"

namespace zim
{
  namespace writer {
    class Dirent;

    class DirentInfo {
      public: // structures
        struct Direct {
          Direct() :
            cluster(nullptr),
            blobNumber(0)
          {};
          Cluster*         cluster;
          blob_index_t     blobNumber;
        } PACKED;

        struct Redirect {
          Redirect(char ns, const std::string& target) :
            targetPath(target),
            ns(ns)
          {};
          Redirect(Redirect&& r) = default;
          ~Redirect() {};
          TinyString targetPath;
          char ns;
        } PACKED;

        struct Resolved {
          Resolved(const Dirent* target) :
            targetDirent(target)
          {};
          const Dirent* targetDirent;
        } PACKED;

      public: // functions
        ~DirentInfo() {
          switch(tag) {
            case DIRECT:
             direct.~Direct();
              break;
            case REDIRECT:
              redirect.~Redirect();
              break;
            case RESOLVED:
              resolved.~Resolved();
              break;
          }
        };
        DirentInfo(Direct&& d):
          direct(std::move(d)),
          tag(DirentInfo::DIRECT)
        {}
        DirentInfo(Redirect&& r):
          redirect(std::move(r)),
          tag(DirentInfo::REDIRECT)
        {}
        DirentInfo(Resolved&& r):
          resolved(std::move(r)),
          tag(DirentInfo::RESOLVED)
        {}
        DirentInfo::Direct& getDirect() {
          ASSERT(tag, ==, DIRECT);
          return direct;
        }
        DirentInfo::Redirect& getRedirect() {
          ASSERT(tag, ==, REDIRECT);
          return redirect;
        }
        DirentInfo::Resolved& getResolved() {
          ASSERT(tag, ==, RESOLVED);
          return resolved;
        }
        const DirentInfo::Direct& getDirect() const {
          ASSERT(tag, ==, DIRECT);
          return direct;
        }
        const DirentInfo::Redirect& getRedirect() const {
          ASSERT(tag, ==, REDIRECT);
          return redirect;
        }
        const DirentInfo::Resolved& getResolved() const {
          ASSERT(tag, ==, RESOLVED);
          return resolved;
        }

      private: // members
        union {
          Direct direct;
          Redirect redirect;
          Resolved resolved;
        } PACKED;

      public: // members
        enum : char {DIRECT, REDIRECT, RESOLVED} tag;
    } PACKED;

    class Dirent
    {
        static const uint16_t redirectMimeType = 0xffff;
        static const uint32_t version = 0;

        PathTitleTinyString pathTitle;
        uint16_t mimeType;
        entry_index_t idx = entry_index_t(0);
        DirentInfo info;
        offset_t offset;
        char ns;
        bool removed;

      public:
        // Creator for a "classic" dirent
        Dirent(char ns, const std::string& path, const std::string& title, uint16_t mimetype);

        // Creator for a "redirection" dirent
        Dirent(char ns, const std::string& path, const std::string& title, char targetNs, const std::string& targetPath);

        // Creator for "temporary" dirent, used to search for dirent in container.
        // We use them in url ordered container so we only need to set the namespace and the path.
        // Other value are irrelevant.
        Dirent(char ns, const std::string& path)
          : Dirent(ns, path, "", 0)
          { }

        char getNamespace() const                { return ns; }
        std::string getTitle() const      { return pathTitle.getTitle(); }
        std::string getRealTitle() const      { return pathTitle.getStoredTitle(); }
        std::string getPath() const       { return pathTitle.getPath(); }

        uint32_t getVersion() const            { return version; }

        char getRedirectNs() const;
        std::string getRedirectPath() const;
        void setRedirect(const Dirent* target) {
          ASSERT(info.tag, ==, DirentInfo::REDIRECT);
          info.~DirentInfo();
          new(&info) DirentInfo(DirentInfo::Resolved(target));
        }
        entry_index_t getRedirectIndex() const      {
          return info.getResolved().targetDirent->getIdx();
        }

        void setIdx(entry_index_t idx_)      { idx = idx_; }
        entry_index_t getIdx() const         { return idx; }


        void setCluster(zim::writer::Cluster* _cluster)
        {
          auto& direct = info.getDirect();
          direct.cluster = _cluster;
          direct.blobNumber = _cluster->count();
        }

        zim::writer::Cluster* getCluster()
        {
          return info.getDirect().cluster;
        }

        cluster_index_t getClusterNumber() const {
          auto& direct = info.getDirect();
          return direct.cluster ? direct.cluster->getClusterIndex() : cluster_index_t(0);
        }
        blob_index_t  getBlobNumber() const {
          return info.getDirect().blobNumber;
        }

        bool isRedirect() const                 { return mimeType == redirectMimeType; }
        bool isItem() const                     { return !isRedirect(); }
        uint16_t getMimeType() const            { return mimeType; }
        void setMimeType(uint16_t m) {
          ASSERT(info.tag, ==, DirentInfo::DIRECT);
          mimeType = m;
        }
        size_t getDirentSize() const
        {
          return (isRedirect() ? 12 : 16) + pathTitle.size() + 1;
        }

        offset_t getOffset() const { return offset; }
        void setOffset(offset_t o) { offset = o; }

        bool isRemoved() const { return removed; }
        void markRemoved() { removed = true; }

        void write(int out_fd) const;

        friend bool compareUrl(const Dirent* d1, const Dirent* d2);
        friend inline bool compareTitle(const Dirent* d1, const Dirent* d2);

      private:
         // A default constructor, used by the pool.
        Dirent();
        friend class DirentPool;
    } PACKED;


    inline bool compareUrl(const Dirent* d1, const Dirent* d2)
    {
      return d1->ns < d2->ns || (d1->ns == d2->ns && d1->getPath() < d2->getPath());
    }
    inline bool compareTitle(const Dirent* d1, const Dirent* d2)
    {
      return d1->ns < d2->ns
        || (d1->ns == d2->ns && d1->getTitle() < d2->getTitle());
    }
  }
}

#endif // ZIM_WRITER_DIRENT_H

