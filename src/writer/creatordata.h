/*
 * Copyright (C) 2018-2021 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2021 Manessh P M <manu.pm55@gmail.com>
 * Copyright (C) 2020 Veloman Yunkan
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

#ifndef ZIM_WRITER_CREATOR_DATA_H
#define ZIM_WRITER_CREATOR_DATA_H

#include <zim/writer/item.h>
#include "queue.h"
#include "_dirent.h"
#include "handler.h"
#include <set>
#include <vector>
#include <map>
#include <fstream>
#include <thread>
#include "config.h"

#include "../fileheader.h"
#include "direntPool.h"
#include "titleListingHandler.h"

namespace zim
{
  namespace writer
  {
    struct UrlCompare {
      bool operator() (const Dirent* d1, const Dirent* d2) const {
        return compareUrl(d1, d2);
      }
    };

    class Cluster;
    class Task;
    class CreatorData
    {
      public:
        typedef std::set<Dirent*, UrlCompare> UrlSortedDirents;
        typedef std::map<std::string, uint16_t> MimeTypesMap;
        typedef std::map<uint16_t, std::string> RMimeTypesMap;
        typedef std::vector<std::string> MimeTypesList;
        typedef std::vector<Cluster*> ClusterList;
        typedef Queue<Cluster*> ClusterQueue;
        typedef Queue<std::shared_ptr<Task>> TaskQueue;
        typedef std::vector<std::thread> ThreadList;

        CreatorData(const std::string& fname, bool verbose,
                       bool withIndex, std::string language,
                       Compression compression,
                       size_t clusterSize);
        virtual ~CreatorData();

        void addDirent(Dirent* dirent);
        void addItemData(Dirent* dirent, std::unique_ptr<ContentProvider> provider, bool compressContent);

        Dirent* createDirent(NS ns, const std::string& path, const std::string& mimetype, const std::string& title);
        Dirent* createItemDirent(const Item* item);
        Dirent* createRedirectDirent(NS ns, const std::string& path, const std::string& title, NS targetNs, const std::string& targetPath);
        Dirent* createCloneDirent(const std::string& path, const std::string& title, const Dirent& target);
        Cluster* closeCluster(bool compressed);

        void setEntryIndexes();
        void resolveRedirectIndexes();
        void resolveMimeTypes();

        uint16_t getMimeTypeIdx(const std::string& mimeType);
        const std::string& getMimeType(uint16_t mimeTypeIdx) const;

        void addError(const std::exception_ptr error);
        bool isErrored() const;
        void quitAllThreads();

        DirentPool  pool;

        UrlSortedDirents   dirents;
        UrlSortedDirents   unresolvedRedirectDirents;
        Dirent*            mainPageDirent;

        MimeTypesMap mimeTypesMap;
        RMimeTypesMap rmimeTypesMap;
        MimeTypesList mimeTypesList;
        uint16_t nextMimeIdx = 0;

        ClusterList clustersList;
        ClusterQueue clusterToWrite;
        TaskQueue taskList;
        ThreadList workerThreads;
        std::thread  writerThread;
        mutable std::mutex m_exceptionLock;
        std::exception_ptr m_exceptionSlot;
        std::atomic<bool> m_errored;
        const Compression compression;
        std::string zimName;
        std::string tmpFileName;
        bool isEmpty = true;
        size_t clusterSize;
        Cluster *compCluster = nullptr;
        Cluster *uncompCluster = nullptr;
        int out_fd;

        bool withIndex;
        std::string indexingLanguage;

        std::shared_ptr<TitleListingHandler> mp_titleListingHandler;
        offset_t m_titleListBlobOffset;  // The offset the title list blob,
                                         // related to the beginning of the start of cluster's data.
        std::vector<std::shared_ptr<DirentHandler>> m_direntHandlers;
        void handle(Dirent* dirent, const Hints& hints = Hints()) {
          for(auto& handler: m_direntHandlers) {
            handler->handle(dirent, hints);
          }
        }
        void handle(Dirent* dirent, std::shared_ptr<Item> item) {
          for(auto& handler: m_direntHandlers) {
            handler->handle(dirent, item);
          }
        }

        // Some stats
        bool verbose;
        entry_index_type nbItems;
        entry_index_type nbRedirectItems;
        entry_index_type nbCompItems;
        entry_index_type nbUnCompItems;
        cluster_index_type nbClusters;
        cluster_index_type nbCompClusters;
        cluster_index_type nbUnCompClusters;
        time_t start_time;

        cluster_index_t clusterCount() const
        { return cluster_index_t(clustersList.size()); }

        entry_index_t itemCount() const
        { return entry_index_t(dirents.size()); }
    };

  }

}

#endif // ZIM_WRITER_CREATOR_DATA_H
