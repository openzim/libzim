/*
 * Copyright (C) 2018-2021 Matthieu Gautier <mgautier@kymeria.fr>
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

    // Be sure that enum value are sorted by "alphabetical" order
    enum class NS: uint8_t {
      C = 0,
      M = 1,
      W = 2,
      X = 3
    };

    char NsAsChar(NS ns);

    class LIBZIM_PRIVATE_API Dirent
    {
        struct Direct {
          Cluster*         cluster;
          blob_index_t     blobNumber;
        } PACKED;

        struct Redirect {
          Dirent* targetDirent;
        } PACKED;

        static const uint16_t redirectMimeType = 0xffff;
        static const uint32_t version = 0;

        PathTitleTinyString pathTitle;
        uint16_t mimeType;
        entry_index_t idx = entry_index_t(0);

        union {
          Direct   direct;
          Redirect redirect;
        } PACKED;

        uint8_t _ns : 2;
        bool removed : 1;
        bool _isFrontArticle : 1;

      private:
        Direct& getDirect()
        {
          ASSERT(isItem(), ==, true);
          return direct;
        }

        const Direct& getDirect() const
        {
          ASSERT(isItem(), ==, true);
          return direct;
        }

      public:
        // Creator for a "classic" dirent
        Dirent(NS ns, const std::string& path, const std::string& title, uint16_t mimetype);

        // Creator for a resolved "redirection" dirent
        Dirent(NS ns, const std::string& path, const std::string& title, Dirent* target);

        // Creator for a "alias" dirent. Reuse the namespace of the targeted
        // Dirent.
        Dirent(const std::string& path, const std::string& title, const Dirent& target);

        // Creator for "temporary" dirent, used to search for dirent in
        // container. We use them in path ordered container so we only need to
        // set the namespace and the path.  Other value are irrelevant.
        Dirent(NS ns, const std::string& path)
          : Dirent(ns, path, "", nullptr)
          { }

        NS getNamespace() const           { return static_cast<NS>(_ns); }
        std::string getTitle() const      { return pathTitle.getTitle(); }
        std::string getPath() const       { return pathTitle.getPath(); }

        uint32_t getVersion() const            { return version; }

        NS getRedirectNs() const;
        std::string getRedirectPath() const;

        Dirent* getRedirectTargetDirent() const {
          ASSERT(isRedirect(), ==, true);
          return redirect.targetDirent;
        }

        entry_index_t getRedirectIndex() const;

        void setIdx(entry_index_t idx_)      { idx = idx_; }
        entry_index_t getIdx() const         { return idx; }


        void setCluster(zim::writer::Cluster* _cluster)
        {
          auto& direct = getDirect();
          direct.cluster = _cluster;
          direct.blobNumber = _cluster->count();
        }

        cluster_index_t getClusterNumber() const {
          auto& direct = getDirect();
          ASSERT(direct.cluster, !=, nullptr);
          return direct.cluster->getClusterIndex();
        }

        blob_index_t  getBlobNumber() const {
          return getDirect().blobNumber;
        }

        bool isRedirect() const      { return mimeType == redirectMimeType; }
        bool isItem() const          { return !isRedirect(); }

        uint16_t getMimeType() const { return mimeType; }

        void setMimeType(uint16_t m) {
          ASSERT(isItem(), ==, true);
          mimeType = m;
        }

        size_t getDirentSize() const
        {
          return (isRedirect() ? 12 : 16) + pathTitle.size() + 1;
        }

        bool isRemoved() const { return removed; }
        void markRemoved() { removed = true; }

        bool isFrontArticle() const { return _isFrontArticle; }
        void markFrontArticle() { _isFrontArticle = true; }

        bool isPlaceholder() const
        {
          return isRedirect()
              && getRedirectTargetDirent() == nullptr;
        }

        bool isUnresolvedRedirect() const
        {
          return isRedirect()
              && getRedirectTargetDirent() != nullptr
              && getRedirectTargetDirent()->isPlaceholder();
        }

        void write(int out_fd) const;

        bool comparePath(const Dirent& other) const {
          return pathTitle.comparePath(other.pathTitle);
        }

        bool compareTitle(const Dirent& other) const {
          return pathTitle.compareTitle(other.pathTitle);
        }
    } PACKED;

    inline bool comparePath(const Dirent& d1, const Dirent& d2)
    {
      return d1.getNamespace() < d2.getNamespace()
        || (d1.getNamespace() == d2.getNamespace() && d1.comparePath(d2));
    }

    inline bool compareTitle(const Dirent* d1, const Dirent* d2)
    {
      return d1->compareTitle(*d2);
    }
  }
}

#endif // ZIM_WRITER_DIRENT_H

