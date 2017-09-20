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
#include <zim/fileheader.h>
#include "cache.h"
#include "dirent.h"
#include "cluster.h"
#include "buffer.h"

namespace zim
{
  class FileReader;
  class FileCompound;
  class FileImpl
  {
      std::shared_ptr<FileCompound> zimFile;
      std::shared_ptr<FileReader> zimReader;
      std::vector<char> bufferDirentZone;
      pthread_mutex_t bufferDirentLock;
      Fileheader header;
      std::string filename;

      std::shared_ptr<const Buffer> titleIndexBuffer;
      std::shared_ptr<const Buffer> urlPtrOffsetBuffer;
      std::shared_ptr<const Buffer> clusterOffsetBuffer;

      Cache<size_type, std::shared_ptr<const Dirent>> direntCache;
      pthread_mutex_t direntCacheLock;

      Cache<offset_type, std::shared_ptr<Cluster>> clusterCache;
      pthread_mutex_t clusterCacheLock;

      bool cacheUncompressedCluster;
      typedef std::map<char, size_type> NamespaceCache;

      NamespaceCache namespaceBeginCache;
      pthread_mutex_t namespaceBeginLock;
      NamespaceCache namespaceEndCache;
      pthread_mutex_t namespaceEndLock;

      typedef std::vector<std::string> MimeTypes;
      MimeTypes mimeTypes;

      offset_type getOffset(const Buffer* buffer, size_type idx);

    public:

      explicit FileImpl(const std::string& fname);

      time_t getMTime() const;

      const std::string& getFilename() const   { return filename; }
      const Fileheader& getFileheader() const  { return header; }
      offset_type getFilesize() const;

      std::shared_ptr<const Dirent> getDirent(size_type idx);
      std::shared_ptr<const Dirent> getDirentByTitle(size_type idx);
      size_type getIndexByTitle(size_type idx);
      size_type getCountArticles() const       { return header.getArticleCount(); }


      std::pair<bool, size_type> findx(char ns, const std::string& url);
      std::pair<bool, size_type> findx(const std::string& url);
      std::pair<bool, size_type> findxByTitle(char ns, const std::string& title);

      std::shared_ptr<const Cluster> getCluster(size_type idx);
      size_type getCountClusters() const       { return header.getClusterCount(); }
      offset_type getClusterOffset(size_type idx)   { return getOffset(clusterOffsetBuffer.get(), idx); }
      offset_type getBlobOffset(size_type clusterIdx, size_type blobIdx);

      size_type getNamespaceBeginOffset(char ch);
      size_type getNamespaceEndOffset(char ch);
      size_type getNamespaceCount(char ns)
        { return getNamespaceEndOffset(ns) - getNamespaceBeginOffset(ns); }

      std::string getNamespaces();
      bool hasNamespace(char ch) const;

      const std::string& getMimeType(uint16_t idx) const;

      std::string getChecksum();
      bool verify();
      bool is_multiPart() const;
  };

}

#endif // ZIM_FILEIMPL_H

