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

#ifndef ZIM_FILEIMPL_H
#define ZIM_FILEIMPL_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <zim/zim.h>
#include <mutex>
#include "lrucache.h"
#include "concurrent_cache.h"
#include "_dirent.h"
#include "dirent_lookup.h"
#include "cluster.h"
#include "buffer.h"
#include "file_reader.h"
#include "file_compound.h"
#include "fileheader.h"
#include "zim_types.h"

namespace zim
{
  class FileImpl
  {
      std::shared_ptr<FileCompound> zimFile;
      std::shared_ptr<Reader> zimReader;
      std::vector<char> bufferDirentZone;
      std::mutex bufferDirentLock;
      Fileheader header;

      std::unique_ptr<const Reader> titleIndexReader;
      std::unique_ptr<const Reader> urlPtrOffsetReader;
      std::unique_ptr<const Reader> clusterOffsetReader;

      lru_cache<entry_index_type, std::shared_ptr<const Dirent>> direntCache;
      std::mutex direntCacheLock;

      typedef std::shared_ptr<const Cluster> ClusterHandle;
      ConcurrentCache<cluster_index_type, ClusterHandle> clusterCache;

      const bool m_newNamespaceScheme;
      const entry_index_t m_startUserEntry;
      const entry_index_t m_endUserEntry;

      typedef std::vector<std::string> MimeTypes;
      MimeTypes mimeTypes;

      using pair_type = std::pair<cluster_index_type, entry_index_type>;
      mutable std::vector<pair_type> articleListByCluster;
      mutable std::once_flag orderOnceFlag;

      using DirentLookup = zim::DirentLookup<FileImpl>;
      mutable std::unique_ptr<DirentLookup> m_direntLookup;

    public:
      using FindxResult = std::pair<bool, entry_index_t>;
      using FindxTitleResult = std::pair<bool, title_index_t>;

      explicit FileImpl(const std::string& fname);

      time_t getMTime() const;

      const std::string& getFilename() const   { return zimFile->filename(); }
      const Fileheader& getFileheader() const  { return header; }
      zsize_t getFilesize() const;
      bool hasNewNamespaceScheme() const { return m_newNamespaceScheme; }

      FileCompound::PartRange getFileParts(offset_t offset, zsize_t size);
      std::shared_ptr<const Dirent> getDirent(entry_index_t idx);
      std::shared_ptr<const Dirent> getDirentByTitle(title_index_t idx);
      entry_index_t getIndexByTitle(title_index_t idx) const;
      entry_index_t getIndexByClusterOrder(entry_index_t idx) const;
      entry_index_t getCountArticles() const { return entry_index_t(header.getArticleCount()); }

      FindxResult findx(char ns, const std::string& url);
      FindxResult findx(const std::string& url);
      FindxTitleResult findxByTitle(char ns, const std::string& title);

      std::shared_ptr<const Cluster> getCluster(cluster_index_t idx);
      cluster_index_t getCountClusters() const       { return cluster_index_t(header.getClusterCount()); }
      offset_t getClusterOffset(cluster_index_t idx) const;
      offset_t getBlobOffset(cluster_index_t clusterIdx, blob_index_t blobIdx);

      entry_index_t getNamespaceBeginOffset(char ch);
      entry_index_t getNamespaceEndOffset(char ch);
      entry_index_t getNamespaceCount(char ns)
        { return getNamespaceEndOffset(ns) - getNamespaceBeginOffset(ns); }

      entry_index_t getStartUserEntry() const { return m_startUserEntry; }
      entry_index_t getEndUserEntry() const { return m_endUserEntry; }
      entry_index_t getUserEntryCount() const { return m_endUserEntry - m_startUserEntry; }

      std::string getNamespaces();
      bool hasNamespace(char ch) const;

      const std::string& getMimeType(uint16_t idx) const;

      std::string getChecksum();
      bool verify();
      bool is_multiPart() const;

      bool checkIntegrity(IntegrityCheck checkType);
  private:
      DirentLookup& direntLookup();
      ClusterHandle readCluster(cluster_index_t idx);
      std::shared_ptr<const Dirent> readDirent(offset_t offset);
      offset_type getMimeListEndUpperLimit() const;
      void readMimeTypes();
      void quickCheckForCorruptFile();

      bool checkChecksum();
      bool checkDirentPtrs();
      bool checkDirentOrder();
      bool checkTitleIndex();
      bool checkClusterPtrs();
  };

}

#endif // ZIM_FILEIMPL_H

