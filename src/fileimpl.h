/*
 * Copyright (C) 2017-2021 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2020-2021 Veloman Yunkan
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

#include <atomic>
#include <string>
#include <tuple>
#include <vector>
#include <memory>
#include <zim/zim.h>
#include <mutex>
#include "concurrent_cache.h"
#include "_dirent.h"
#include "dirent_accessor.h"
#include "dirent_lookup.h"
#include "cluster.h"
#include "file_reader.h"
#include "file_compound.h"
#include "fileheader.h"
#include "zim_types.h"
#include "direntreader.h"

#ifdef ENABLE_XAPIAN
#include "search_internal.h"
#endif

namespace zim
{
  class FileImpl;
  typedef std::shared_ptr<const Cluster> ClusterHandle;
  typedef std::tuple<const FileImpl*, cluster_index_type> ClusterRef;
  typedef ConcurrentCache<ClusterRef, ClusterHandle, ClusterMemorySize> ClusterCache;
  ClusterCache& getClusterCache();

  class FileImpl
  {
      std::shared_ptr<FileCompound> zimFile;
      std::shared_ptr<Reader> zimReader;
      std::shared_ptr<DirentReader> direntReader;
      Fileheader header;

      std::unique_ptr<const Reader> clusterOffsetReader;

      std::shared_ptr<const DirectDirentAccessor> mp_pathDirentAccessor;
      std::unique_ptr<const IndirectDirentAccessor> mp_titleDirentAccessor;

      const bool m_hasFrontArticlesIndex;
      const entry_index_t m_startUserEntry;
      const entry_index_t m_endUserEntry;

      typedef std::vector<std::string> MimeTypes;
      MimeTypes mimeTypes;

      mutable std::vector<entry_index_type> m_articleListByCluster;
      mutable std::mutex m_articleListByClusterMutex;

      struct DirentLookupConfig
      {
        typedef DirectDirentAccessor DirentAccessorType;
        typedef entry_index_t index_t;
        static const std::string& getDirentKey(const Dirent& d) {
          return d.getPath();
        }
      };

      using DirentLookup = zim::DirentLookup<DirentLookupConfig>;
      using FastDirentLookup = zim::FastDirentLookup<DirentLookupConfig>;
      std::unique_ptr<DirentLookup> m_direntLookup;


      struct ByTitleDirentLookupConfig
      {
        typedef IndirectDirentAccessor DirentAccessorType;
        typedef title_index_t index_t;
        static const std::string& getDirentKey(const Dirent& d) {
          return d.getTitle();
        }
      };

      using ByTitleDirentLookup = zim::DirentLookup<ByTitleDirentLookupConfig>;
      std::unique_ptr<ByTitleDirentLookup> m_byTitleDirentLookup;

#ifdef ENABLE_XAPIAN
      std::shared_ptr<XapianDb> mp_xapianDb;
      mutable std::mutex m_xapianDbCreationMutex;
      mutable std::atomic_bool m_xapianDbCreated;
#endif

    public:
      using FindxResult = std::pair<bool, entry_index_t>;
      using FindxTitleResult = std::pair<bool, title_index_t>;

      FileImpl(const std::string& fname, OpenConfig openConfig);
#ifndef _WIN32
      FileImpl(int fd, OpenConfig openConfig);
      FileImpl(FdInput fd, OpenConfig openConfig);
      FileImpl(const std::vector<FdInput>& fds, OpenConfig openConfig);
#endif
      ~FileImpl();

      time_t getMTime() const;

      const std::string& getFilename() const   { return zimFile->filename(); }
      const Fileheader& getFileheader() const  { return header; }
      zsize_t getFilesize() const;
      bool hasNewNamespaceScheme() const { return header.useNewNamespaceScheme(); }
      bool hasFrontArticlesIndex() const { return m_hasFrontArticlesIndex; }

      FileCompound::PartRange getFileParts(offset_t offset, zsize_t size) const;
      std::shared_ptr<const Dirent> getDirent(entry_index_t idx) const;
      std::shared_ptr<const Dirent> getDirentByTitle(title_index_t idx) const;
      entry_index_t getIndexByTitle(title_index_t idx) const;
      entry_index_t getIndexByClusterOrder(entry_index_t idx) const;
      entry_index_t getCountArticles() const { return entry_index_t(header.getArticleCount()); }

      FindxResult findx(char ns, const std::string &path) const;
      FindxResult findx(const std::string &path) const;
      FindxResult findxMetadata(const std::string &name) const;
      FindxTitleResult findxByTitle(char ns, const std::string& title);

      Blob getBlob(const Dirent& dirent, offset_t offset = offset_t(0)) const;
      Blob getBlob(const Dirent& dirent, offset_t offset, zsize_t size) const;

      std::shared_ptr<const Cluster> getCluster(cluster_index_t idx) const;
      cluster_index_t getCountClusters() const       { return cluster_index_t(header.getClusterCount()); }
      offset_t getClusterOffset(cluster_index_t idx) const;
      offset_t getBlobOffset(cluster_index_t clusterIdx, blob_index_t blobIdx) const;
      ItemDataDirectAccessInfo getDirectAccessInformation(cluster_index_t clusterIdx, blob_index_t blobIdx) const;

      entry_index_t getNamespaceBeginOffset(char ch) const;
      entry_index_t getNamespaceEndOffset(char ch) const;
      entry_index_t getNamespaceEntryCount(char ch) const {
        return getNamespaceEndOffset(ch) - getNamespaceBeginOffset(ch);
      }

      entry_index_t getStartUserEntry() const { return m_startUserEntry; }
      entry_index_t getEndUserEntry() const { return m_endUserEntry; }
      // The number of entries added by the creator. (So excluding index, ...).
      // On new namespace scheme, number of entries in C namespace
      entry_index_t getUserEntryCount() const { return m_endUserEntry - m_startUserEntry; }
      // The number of enties that can be considered as front article (no resource)
      entry_index_t getFrontEntryCount() const;

      const std::string& getMimeType(uint16_t idx) const;

      std::string getChecksum();
      bool verify();
      bool is_multiPart() const;

      bool checkIntegrity(IntegrityCheck checkType);

      size_t getDirentCacheMaxSize() const;
      size_t getDirentCacheCurrentSize() const;
      void setDirentCacheMaxSize(size_t nbDirents);

#ifdef ENABLE_XAPIAN
      std::shared_ptr<XapianDb> loadXapianDb();
      std::shared_ptr<XapianDb> getXapianDb();
#endif
  private:
      FileImpl(std::shared_ptr<FileCompound> zimFile, OpenConfig openConfig);
      FileImpl(std::shared_ptr<FileCompound> zimFile, offset_t offset, zsize_t size, OpenConfig openConfig);

      void dropCachedClusters() const;

      std::unique_ptr<IndirectDirentAccessor> getTitleAccessorV1(const entry_index_t idx);
      std::unique_ptr<IndirectDirentAccessor> getTitleAccessor(const offset_t offset, const zsize_t size, const std::string& name);

      void prepareArticleListByCluster() const;
      DirentLookup& direntLookup() const;
      ClusterHandle readCluster(cluster_index_t idx) const;
      offset_type getMimeListEndUpperLimit() const;
      void readMimeTypes();
      void quickCheckForCorruptFile();

      bool checkChecksum();
      bool checkDirentPtrs();
      bool checkDirentOrder();
      bool checkTitleIndex();
      bool checkClusterPtrs();
      bool checkClusters();
      bool checkDirentMimeTypes();
  };

}

#endif // ZIM_FILEIMPL_H

