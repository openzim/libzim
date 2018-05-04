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

#ifndef ZIM_WRITER_ZIMCREATOR_IMPL_H
#define ZIM_WRITER_ZIMCREATOR_IMPL_H

#include <zim/writer/articlesource.h>
#include <zim/fileheader.h>
#include "dirent.h"
#include <vector>
#include <map>
#include <fstream>

namespace zim
{
  namespace writer
  {
    class Cluster;
    class ZimCreatorImpl
    {
      public:
        typedef std::vector<Dirent> DirentsType;
        typedef std::vector<DirentsType::size_type> DirentPtrsType;
        typedef std::vector<article_index_t> ArticleIdxVectorType;
        typedef std::vector<offset_t> OffsetsType;
        typedef std::map<std::string, uint16_t> MimeTypes;
        typedef std::map<uint16_t, std::string> RMimeTypes;

      private:
        size_t minChunkSize;

        Fileheader header;

        DirentsType dirents;
        ArticleIdxVectorType titleIdx;
        OffsetsType clusterOffsets;
        MimeTypes mimeTypes;
        RMimeTypes rmimeTypes;
        uint16_t nextMimeIdx;
        CompressionType compression;
        bool isEmpty;
        bool isExtended;
        zsize_t clustersSize;
        DirentPtrsType compDirents, uncompDirents;
        Cluster *compCluster, *uncompCluster;
        std::ofstream tmp_out;

        // Some stats
        bool verbose;
        article_index_type nbArticles;
        article_index_type nbCompArticles;
        article_index_type nbUnCompArticles;
        article_index_type nbFileArticles;
        cluster_index_type nbClusters;
        cluster_index_type nbCompClusters;
        cluster_index_type nbUnCompClusters;

        Dirent createDirentFromArticle(const Article* article);
        void closeCluster(bool compressed);
        void addDirent(const Dirent& dirent, const Article* article);
        void createDirentsAndClusters(ArticleSource& src, const std::string& tmpfname);
        void createTitleIndex(ArticleSource& src);
        void fillHeader(ArticleSource& src);
        void write(const std::string& fname, const std::string& tmpfname);

        cluster_index_t clusterCount() const        { return cluster_index_t(clusterOffsets.size()); }
        article_index_t articleCount() const        { return article_index_t(dirents.size()); }
        zsize_t mimeListSize() const;
        offset_t mimeListPos() const
        { return offset_t(Fileheader::size); }

        zsize_t  urlPtrSize() const
        { return zsize_t(article_index_type(articleCount()) * sizeof(offset_type)); }

        offset_t urlPtrPos() const
        { return mimeListPos() + mimeListSize(); }

        zsize_t  titleIdxSize() const
        { return zsize_t(article_index_type(articleCount()) * sizeof(article_index_type)); }

        offset_t titleIdxPos() const
        { return urlPtrPos() + urlPtrSize(); }

        zsize_t  indexSize() const;

        offset_t indexPos() const
        { return titleIdxPos() + titleIdxSize(); }

        zsize_t  clusterPtrSize() const
        { return zsize_t(cluster_index_type(clusterCount()) * sizeof(offset_type)); }

        offset_t clusterPtrPos() const
        { return indexPos() + indexSize(); }

        offset_t checksumPos() const
        { return clusterPtrPos() + clusterPtrSize() + clustersSize; }

        uint16_t getMimeTypeIdx(const std::string& mimeType);
        const std::string& getMimeType(uint16_t mimeTypeIdx) const;

      public:
        ZimCreatorImpl(bool verbose);
        ZimCreatorImpl(int& argc, char* argv[]);
        virtual ~ZimCreatorImpl();

        zsize_t getMinChunkSize()    { return zsize_t(minChunkSize); }
        void setMinChunkSize(zsize_t s)   { minChunkSize = s.v; }

        void create(const std::string& fname, ArticleSource& src);
    };

  }

}

#endif // ZIM_WRITER_ZIMCREATOR_IMPL_H
