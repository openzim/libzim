/*
 * Copyright (C) 2006,2009 Tommi Maekitalo
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

#ifndef ZIM_FILE_H
#define ZIM_FILE_H

#include <string>
#include <iterator>
#include <memory>
#include "zim.h"
#include "article.h"
#include "blob.h"
#include "fileheader.h"

class ZimDumper;

namespace zim
{
  class Search;
  class FileImpl;
  class Cluster;

  class File
  {
    friend class ::ZimDumper;
    std::shared_ptr<FileImpl> impl;

    public:
      File()
        { }
      explicit File(const std::string& fname);

      const std::string& getFilename() const;
      const Fileheader& getFileheader() const;
      offset_type getFilesize() const;

      size_type getCountArticles() const;

      Article getArticle(size_type idx) const;
      Article getArticle(char ns, const std::string& url) const;
      Article getArticleByUrl(const std::string& url) const;
      Article getArticleByTitle(size_type idx) const;
      Article getArticleByTitle(char ns, const std::string& title) const;

      std::shared_ptr<const Cluster> getCluster(size_type idx) const;
      size_type getCountClusters() const;
      offset_type getClusterOffset(size_type idx) const;

      Blob getBlob(size_type clusterIdx, size_type blobIdx) const;
      offset_type getOffset(size_type clusterIdx, size_type blobIdx) const;

      size_type getNamespaceBeginOffset(char ch) const;
      size_type getNamespaceEndOffset(char ch) const;
      size_type getNamespaceCount(char ns) const;

      std::string getNamespaces() const;
      bool hasNamespace(char ch) const;

      class const_iterator;

      const_iterator begin() const;
      const_iterator beginByTitle() const;
      const_iterator end() const;
      const_iterator findByTitle(char ns, const std::string& title) const;
      const_iterator find(char ns, const std::string& url) const;
      const_iterator find(const std::string& url) const;

      const Search* search(const std::string& query, int start, int end) const;
      const Search* suggestions(const std::string& query, int start, int end) const;

      time_t getMTime() const;

      const std::string& getMimeType(uint16_t idx) const;

      std::string getChecksum();
      bool verify();

      bool is_multiPart() const;
  };

  std::string urldecode(const std::string& url);

}

#endif // ZIM_FILE_H

