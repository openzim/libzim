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

#include "config.h"

#include "zimcreatordata.h"
#include "cluster.h"
#include <zim/blob.h>
#include <zim/writer/zimcreator.h>
#include "../endian_tools.h"
#include <algorithm>
#include <fstream>

#if defined(ENABLE_XAPIAN)
  #include "xapianIndexer.h"
#endif

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <limits>
#include <stdexcept>
#include <sstream>
#include "md5stream.h"
#include "tee.h"
#include "log.h"

log_define("zim.writer.creator")

#define INFO(e) \
    do { \
        log_info(e); \
        std::cout << e << std::endl; \
    } while(false)

namespace zim
{
  namespace writer
  {
    ZimCreatorData::ZimCreatorData(const std::string& fname,
                                   bool verbose,
                                   bool withIndex,
                                   std::string language)
      : minChunkSize(1024-64),
        nextMimeIdx(0),
        compression(zimcompLzma),
        isEmpty(true),
        isExtended(false),
        compCluster(nullptr),
        uncompCluster(nullptr),
        withIndex(withIndex),
        indexingLanguage(language),
        verbose(verbose)
    {
      basename =  (fname.size() > 4 && fname.compare(fname.size() - 4, 4, ".zim") == 0)
                        ? fname.substr(0, fname.size() - 4)
                        : fname;
      tmpfname = basename + ".tmp";
      tmp_out = std::ofstream(tmpfname);

      // We keep both a "compressed cluster" and an "uncompressed cluster"
      // because we don't know which one will fill up first.  We also need
      // to track the dirents currently in each, so we can fix up the
      // cluster index if the other one ends up written first.
      compCluster = new Cluster(compression);
      uncompCluster = new Cluster(zimcompNone);

#if defined(ENABLE_XAPIAN)
      if (withIndex) {
          indexer = new XapianIndexer(indexingLanguage, true);
          indexer->indexingPrelude(tmpfname+".idx");
      }
#endif
    }

    ZimCreatorData::~ZimCreatorData()
    {
      if (compCluster)
        delete compCluster;
      if (uncompCluster)
        delete uncompCluster;
      ::remove(tmpfname.c_str());
#if defined(ENABLE_XAPIAN)
      if (indexer)
        delete indexer;
#endif
    }

    Dirent ZimCreatorData::createDirentFromArticle(const Article* article)
    {
      Dirent dirent;
      dirent.setAid(article->getAid());
      dirent.setUrl(article->getNamespace(), article->getUrl());
      dirent.setTitle(article->getTitle());
      dirent.setParameter(article->getParameter());

      log_debug("article " << dirent.getLongUrl() << " fetched");

      if (article->isRedirect())
      {
        dirent.setRedirect(article_index_t(0));
        dirent.setRedirectAid(article->getRedirectAid());
        log_debug("is redirect to " << dirent.getRedirectAid());
      }
      else if (article->isLinktarget())
      {
        dirent.setLinktarget();
      }
      else if (article->isDeleted())
      {
        dirent.setDeleted();
      }
      else
      {
        dirent.setArticle(getMimeTypeIdx(article->getMimeType()), cluster_index_t(0), blob_index_t(0));
        dirent.setCompress(article->shouldCompress());
        log_debug("is article; mimetype " << dirent.getMimeType());
      }
      return dirent;
    }


    void ZimCreatorData::closeCluster(bool compressed)
    {
      Cluster *cluster;
      DirentPtrsType *myDirents, *otherDirents;
      nbClusters++;
      if (compressed )
      {
        cluster = compCluster;
        myDirents = &compDirents;
        otherDirents = &uncompDirents;
        nbCompClusters++;
      } else {
        cluster = uncompCluster;
        myDirents = &uncompDirents;
        otherDirents = &compDirents;
        nbUnCompClusters++;
      }
      clusterOffsets.push_back(offset_t(tmp_out.tellp()));
      tmp_out << *cluster;
      log_debug("cluster written");
      if (cluster->is_extended() )
        isExtended = true;
      cluster->clear();
      myDirents->clear();
      // Update the cluster number of the dirents *not* written to disk.
      for (DirentPtrsType::iterator dpi = otherDirents->begin();
           dpi != otherDirents->end(); ++dpi)
      {
        Dirent *di = &dirents[*dpi];
        di->setCluster(cluster_index_t(clusterOffsets.size()), di->getBlobNumber());
      }
    }

    void ZimCreatorData::addDirent(const Dirent& dirent, const Article* article)
    {
      dirents.push_back(dirent);

      // If this is a redirect, we're done: there's no blob to add.
      if (dirent.isRedirect())
      {
        return;
      }

      // Add blob data to compressed or uncompressed cluster.
      auto articleSize = article->getSize();
      if (articleSize > 0)
      {
        isEmpty = false;
      }

      Cluster *cluster;
      DirentPtrsType *myDirents;
      if (dirent.isCompress())
      {
        cluster = compCluster;
        myDirents = &compDirents;
      }
      else
      {
        cluster = uncompCluster;
        myDirents = &uncompDirents;
      }

      // If cluster will be too large, write it to dis, and open a new
      // one for the content.
      if ( cluster->count()
        && cluster->size().v+articleSize >= minChunkSize * 1024
         )
      {
        log_info("cluster with " << cluster->count() << " articles, " <<
                 cluster->size() << " bytes; current title \"" <<
                 dirent.getTitle() << '\"');
        closeCluster(dirent.isCompress());
      }

      dirents.back().setCluster(cluster_index_t(clusterOffsets.size()), cluster->count());
      cluster->addArticle(article);
      myDirents->push_back(dirents.size()-1);
    }

    void ZimCreator::addArticle(const Article& article)
    {
      Dirent dirent = data->createDirentFromArticle(&article);
      data->addDirent(dirent, &article);
      data->nbArticles++;
      if (article.shouldCompress())
        data->nbCompArticles++;
      else
        data->nbUnCompArticles++;
      if (!article.getFilename().empty())
        data->nbFileArticles++;
      if (article.shouldIndex())
        data->nbIndexArticles++;

      if (verbose && data->nbArticles%1000 == 0){
        std::cout << "A:" << data->nbArticles
                  << "; CA:" << data->nbCompArticles
                  << "; UA:" << data->nbUnCompArticles
                  << "; FA:" << data->nbFileArticles
                  << "; IA:" << data->nbIndexArticles
                  << "; C:" << data->nbClusters
                  << "; CC:" << data->nbCompClusters
                  << "; UC:" << data->nbUnCompClusters
                  << std::endl;
      }

#if defined(ENABLE_XAPIAN)
      if(withIndex && article.shouldIndex()) {
        data->indexer->index(&article);
      }
#endif
    }

    void ZimCreator::finishZimCreation()
    {
      if (verbose) {
        std::cout << "A:" << data->nbArticles
                  << "; CA:" << data->nbCompArticles
                  << "; UA:" << data->nbUnCompArticles
                  << "; FA:" << data->nbFileArticles
                  << "; IA:" << data->nbIndexArticles
                  << "; C:" << data->nbClusters
                  << "; CC:" << data->nbCompClusters
                  << "; UC:" << data->nbUnCompClusters
                  << std::endl;
      }

#if defined(ENABLE_XAPIAN)
      if (withIndex) {
          data->indexer->indexingPostlude();
          usleep(100);
          auto article = data->indexer->getMetaArticle();
          Dirent dirent = data->createDirentFromArticle(article);
          data->addDirent(dirent, article);
          delete article;
      }
#endif

      // When we've seen all articles, write any remaining clusters.
      if (data->compCluster->count())
        data->closeCluster(true);

      if (data->uncompCluster->count())
        data->closeCluster(false);

      if (!data->tmp_out)
      {
        throw std::runtime_error("failed to write temporary cluster file");
      }

      data->clustersSize = zsize_t(data->tmp_out.tellp());

      data->tmp_out.close();

      data->removeInvalidRedirects();
      data->setArticleIndexes();
      data->resolveRedirectIndexes();

      data->resolveMimeTypes();

      INFO("create title index");
      data->createTitleIndex();
      INFO(data->dirents.size() << " title index created");
      INFO(data->clusterOffsets.size() << " clusters created");

      INFO("fill header");
      Fileheader header;
      fillHeader(&header);

      // sort
      log_debug("sort " << dirents.size() << " directory entries (url)");
      std::sort(data->dirents.begin(), data->dirents.end(), compareUrl);

      INFO("write zimfile");
      write(header, data->basename + ".zim");

      INFO("ready");
    }

    namespace
    {
      class CompareTitle
      {
          ZimCreatorData::DirentsType& dirents;

        public:
          explicit CompareTitle(ZimCreatorData::DirentsType& dirents_)
            : dirents(dirents_)
            { }
          bool operator() (article_index_t titleIdx1, article_index_t titleIdx2) const
          {
            Dirent d1 = dirents[article_index_type(titleIdx1)];
            Dirent d2 = dirents[article_index_type(titleIdx2)];
            return d1.getNamespace() < d2.getNamespace()
               || (d1.getNamespace() == d2.getNamespace()
                && d1.getTitle() < d2.getTitle());
          }
      };
    }

    void ZimCreatorData::createTitleIndex()
    {
      titleIdx.resize(0);
      titleIdx.reserve(dirents.size());
      for (auto dirent: dirents)
        titleIdx.push_back(dirent.getIdx());

      CompareTitle compareTitle(dirents);
      std::sort(titleIdx.begin(), titleIdx.end(), compareTitle);
    }

    void ZimCreator::fillHeader(Fileheader* header)
    {
      std::string mainAid = getMainPage();
      std::string layoutAid = getLayoutPage();

      log_debug("main aid=" << mainAid << " layout aid=" << layoutAid);

      if (data->isExtended) {
        header->setMajorVersion(Fileheader::zimExtendedMajorVersion);
      } else {
        header->setMajorVersion(Fileheader::zimClassicMajorVersion);
      }
      header->setMinorVersion(Fileheader::zimMinorVersion);
      header->setMainPage(std::numeric_limits<article_index_type>::max());
      header->setLayoutPage(std::numeric_limits<article_index_type>::max());

      if (!mainAid.empty() || !layoutAid.empty())
      {
        for (auto& dirent: data->dirents)
        {
          if (mainAid == dirent.getAid())
          {
            log_debug("main idx=" << dirent.getIdx());
            header->setMainPage(article_index_type(dirent.getIdx()));
          }

          if (layoutAid == dirent.getAid())
          {
            log_debug("layout idx=" << dirent.getIdx());
            header->setLayoutPage(article_index_type(dirent.getIdx()));
          }
        }
      }

      header->setUuid( getUuid() );
      header->setArticleCount( data->dirents.size() );

      offset_type offset(Fileheader::size);
      header->setMimeListPos( offset );

      offset += data->mimeListSize().v;
      header->setUrlPtrPos( offset );

      offset += data->urlPtrSize().v;
      header->setTitleIdxPos( offset );
      header->setClusterCount( data->clusterOffsets.size() );

      offset += data->titleIdxSize().v + data->indexSize().v;
      header->setClusterPtrPos( offset );

      offset += data->clusterPtrSize().v + data->clustersSize.v;
      header->setChecksumPos( offset );
    }

    void ZimCreatorData::resolveMimeTypes()
    {
      std::vector<std::string> oldMImeList;
      std::vector<uint16_t> mapping;

      for (auto& rmimeType: rmimeTypesMap)
      {
        oldMImeList.push_back(rmimeType.second);
        mimeTypesList.push_back(rmimeType.second);
      }

      mapping.resize(oldMImeList.size());
      std::sort(mimeTypesList.begin(), mimeTypesList.end());

      for (unsigned i=0; i<oldMImeList.size(); ++i)
      {
        for (unsigned j=0; j<mimeTypesList.size(); ++j)
        {
          if (oldMImeList[i] == mimeTypesList[j])
            mapping[i] = static_cast<uint16_t>(j);
        }
      }

      for (auto& dirent: dirents)
      {
        if (dirent.isArticle())
          dirent.setMimeType(mapping[dirent.getMimeType()]);
      }
    }

    void ZimCreator::write(const Fileheader& header, const std::string& fname) const
    {
      std::ofstream zimfile(fname);
      Md5stream md5;
      Tee out(zimfile, md5);

      out << header;

      log_debug("after writing header - pos=" << zimfile.tellp());

      // write mime type list
      for(auto& mimeType: data->mimeTypesList)
      {
        out << mimeType << '\0';
      }

      out << '\0';

      // write url ptr list
      offset_t off(header.getTitleIdxPos() + data->titleIdxSize().v);
      for (auto& dirent: data->dirents)
      {
        char tmp_buff[sizeof(offset_type)];
        toLittleEndian(off.v, tmp_buff);
        out.write(tmp_buff, sizeof(offset_type));
        off += dirent.getDirentSize();
      }

      log_debug("after writing direntPtr - pos=" << out.tellp());

      // write title index
      for (auto titleid: data->titleIdx)
      {
        char tmp_buff[sizeof(article_index_type)];
        toLittleEndian(titleid.v, tmp_buff);
        out.write(tmp_buff, sizeof(article_index_type));
      }

      log_debug("after writing fileIdxList - pos=" << out.tellp());

      // write directory entries
      for (auto& dirent: data->dirents)
      {
        out << dirent;
        log_debug("write " << dirent.getTitle() << " dirent.size()=" << dirent.getDirentSize() << " pos=" << out.tellp());
      }

      log_debug("after writing dirents - pos=" << out.tellp());

      // write cluster offset list
      off += data->clusterPtrSize();
      for (auto clusterOffset : data->clusterOffsets)
      {
        offset_t o(off + clusterOffset);
        char tmp_buff[sizeof(offset_type)];
        toLittleEndian(o.v, tmp_buff);
        out.write(tmp_buff, sizeof(offset_type));
      }

      log_debug("after writing clusterOffsets - pos=" << out.tellp());

      // write cluster data

      if (!data->isEmpty)
      {
        std::ifstream clustersFile(data->tmpfname.c_str());
        out << clustersFile.rdbuf();
      }
      else
        log_warn("no data found");

      if (!out)
        throw std::runtime_error("failed to write zimfile");

      log_debug("after writing clusterData - pos=" << out.tellp());
      unsigned char digest[16];
      md5.getDigest(digest);
      zimfile.write(reinterpret_cast<const char*>(digest), 16);
    }

    zsize_t ZimCreatorData::mimeListSize() const
    {
      size_type ret = 1;
      for (auto& rmimeType: rmimeTypesMap)
        ret += (rmimeType.second.size() + 1);
      return zsize_t(ret);
    }

    zsize_t ZimCreatorData::indexSize() const
    {
      size_type s = 0;

      for (auto& dirent: dirents)
        s += dirent.getDirentSize();

      return zsize_t(s);
    }

    uint16_t ZimCreatorData::getMimeTypeIdx(const std::string& mimeType)
    {
      auto it = mimeTypesMap.find(mimeType);
      if (it == mimeTypesMap.end())
      {
        if (nextMimeIdx >= std::numeric_limits<uint16_t>::max())
          throw std::runtime_error("too many distinct mime types");
        mimeTypesMap[mimeType] = nextMimeIdx;
        rmimeTypesMap[nextMimeIdx] = mimeType;
        return nextMimeIdx++;
      }

      return it->second;
    }

    const std::string& ZimCreatorData::getMimeType(uint16_t mimeTypeIdx) const
    {
      auto it = rmimeTypesMap.find(mimeTypeIdx);
      if (it == rmimeTypesMap.end())
        throw std::runtime_error("mime type index not found");
      return it->second;
    }

    void ZimCreatorData::removeInvalidRedirects()
    {
      // sort
      INFO("sort " << dirents.size() << " directory entries (aid)");
      std::sort(dirents.begin(), dirents.end(), compareAid);

      // remove invalid redirects
      INFO("remove invalid redirects from " << dirents.size() << " directory entries");
      ZimCreatorData::DirentsType::size_type di = 0;
      while (di < dirents.size())
      {
        if (di % 10000 == 0)
          INFO(di << "directory entries checked for invalid redirects");

        if (dirents[di].isRedirect())
        {
          log_debug("check " << dirents[di].getTitle() << " redirect to " << dirents[di].getRedirectAid() << " (" << di << '/' << dirents.size() << ')');

          if (!std::binary_search(dirents.begin(), dirents.end(), Dirent(dirents[di].getRedirectAid()), compareAid))
          {
            log_debug("remove invalid redirection " << dirents[di].getTitle());
            dirents.erase(dirents.begin() + di);
            continue;
          }
        }

        ++di;
      }
    }

    void ZimCreatorData::setArticleIndexes()
    {
      // sort
      INFO("sort " << dirents.size() << " directory entries (url)");
      std::sort(dirents.begin(), dirents.end(), compareUrl);

      // set index
      INFO("set index");
      article_index_t idx(0);
      for (auto& dirent: dirents) {
        dirent.setIdx(idx);
        idx += 1;
      }
    }

    void ZimCreatorData::resolveRedirectIndexes()
    {
      // sort
      log_debug("sort " << dirents.size() << " directory entries (aid)");
      std::sort(dirents.begin(), dirents.end(), compareAid);

      // translate redirect aid to index
      INFO("translate redirect aid to index");
      for (auto& di: dirents)
      {
        if (di.isRedirect())
        {
          auto ddi = std::lower_bound(dirents.begin(), dirents.end(), di.getRedirectAid(), compareAid);
          if (ddi != dirents.end() && ddi->getAid() == di.getRedirectAid())
          {
            log_debug("redirect aid=" << ddi->getAid() << " redirect index=" << ddi->getIdx());
            di.setRedirect(ddi->getIdx());
          }
          else
          {
            std::ostringstream msg;
            msg << "internal error: redirect aid " << di.getRedirectAid() << " not found";
            log_fatal(msg.str());
            throw std::runtime_error(msg.str());
          }
        }
      }
    }

    ZimCreator::ZimCreator(bool verbose)
      : verbose(verbose),
        withIndex(false),
        minChunkSize(1024-64)
    {}

    ZimCreator::~ZimCreator() = default;

    size_type ZimCreator::getMinChunkSize() const
    {
      return minChunkSize;
    }

    void ZimCreator::setMinChunkSize(size_type s)
    {
      minChunkSize = s;
    }

    void ZimCreator::setIndexing(bool indexing, std::string language)
    {
      withIndex = indexing;
      indexingLanguage = language;
    }

    Uuid ZimCreator::getUuid()
    {
      return Uuid::generate();
    }

    std::string ZimCreator::getMainPage()
    {
      return std::string();
    }

    std::string ZimCreator::getLayoutPage()
    {
      return std::string();
    }

    void ZimCreator::startZimCreation(const std::string& fname)
    {
      data = std::unique_ptr<ZimCreatorData>(new ZimCreatorData(fname, verbose, withIndex, indexingLanguage));
      data->setMinChunkSize(minChunkSize);
    }
  }
}
