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
#include <pthread.h>
#include <zim/zim.h>
#include <zim/fileheader.h>
#include <mutex>
#include "lrucache.h"
#include "concurrent_cache.h"
#include "_dirent.h"
#include "dirent_lookup.h"
#include "cluster.h"
#include "buffer.h"
#include "file_reader.h"
#include "file_compound.h"
#include "zim_types.h"

namespace zim
{
  class FileImpl
  {
      std::shared_ptr<FileCompound> zimFile;
      std::shared_ptr<FileReader> zimReader;
      std::vector<char> bufferDirentZone;
      pthread_mutex_t bufferDirentLock;
      Fileheader header;
      std::string filename;

      std::unique_ptr<const Reader> titleIndexReader;
      std::unique_ptr<const Reader> urlPtrOffsetReader;
      std::unique_ptr<const Reader> clusterOffsetReader;

      lru_cache<article_index_type, std::shared_ptr<const Dirent>> direntCache;
      pthread_mutex_t direntCacheLock;

      typedef std::shared_ptr<const Cluster> ClusterHandle;
      ConcurrentCache<cluster_index_type, ClusterHandle> clusterCache;

      bool cacheUncompressedCluster;

      typedef std::vector<std::string> MimeTypes;
      MimeTypes mimeTypes;

      using pair_type = std::pair<cluster_index_type, article_index_type>;
      std::vector<pair_type> articleListByCluster;
      std::once_flag orderOnceFlag;

      using DirentLookup = zim::DirentLookup<FileImpl>;
      mutable std::unique_ptr<DirentLookup> m_direntLookup;

    public:
      explicit FileImpl(const std::string& fname);

      time_t getMTime() const;

      const std::string& getFilename() const   { return filename; }
      const Fileheader& getFileheader() const  { return header; }
      zsize_t getFilesize() const;

      FileCompound::PartRange getFileParts(offset_t offset, zsize_t size);
      std::shared_ptr<const Dirent> getDirent(article_index_t idx);
      std::shared_ptr<const Dirent> getDirentByTitle(article_index_t idx);
      article_index_t getIndexByTitle(article_index_t idx);
      article_index_t getCountArticles() const { return article_index_t(header.getArticleCount()); }

      std::pair<bool, article_index_t> findx(char ns, const std::string& url);
      std::pair<bool, article_index_t> findx(const std::string& url);
      std::pair<bool, article_index_t> findxByTitle(char ns, const std::string& title);
      std::pair<bool, article_index_t> findxByClusterOrder(article_index_type idx);

      std::shared_ptr<const Cluster> getCluster(cluster_index_t idx);
      cluster_index_t getCountClusters() const       { return cluster_index_t(header.getClusterCount()); }
      offset_t getClusterOffset(cluster_index_t idx) const;
      offset_t getBlobOffset(cluster_index_t clusterIdx, blob_index_t blobIdx);

      article_index_t getNamespaceBeginOffset(char ch);
      article_index_t getNamespaceEndOffset(char ch);
      article_index_t getNamespaceCount(char ns)
        { return getNamespaceEndOffset(ns) - getNamespaceBeginOffset(ns); }

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
      void readMimeTypes();
      void quickCheckForCorruptFile();

      bool checkChecksum();
      bool checkDirentPtrs();
      bool checkTitleIndex();
  };

}

#endif // ZIM_FILEIMPL_H

