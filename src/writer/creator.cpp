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

#include "creatordata.h"
#include "cluster.h"
#include "debug.h"
#include <zim/blob.h>
#include <zim/writer/creator.h>
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
#include <ctime>
#include "md5stream.h"
#include "tee.h"
#include "log.h"
#include "../fs.h"
#include "../tools.h"

log_define("zim.writer.creator")

#define INFO(e) \
    do { \
        log_info(e); \
        std::cout << e << std::endl; \
    } while(false)

#define TINFO(e) \
    if (verbose) { \
        double seconds = difftime(time(NULL), data->start_time); \
        std::cout << "T:" << (int)(seconds) \
                  << "; " << e << std::endl; \
    }

namespace zim
{
  namespace writer
  {
    void* Creator::clusterWriter(void* arg) {
      auto creator = static_cast<zim::writer::Creator*>(arg);
      zim::writer::Cluster* clusterToWrite;
      unsigned int wait = 0;

      while(true) {
        microsleep(wait);
        if (creator->data->clustersToWrite.popFromQueue(clusterToWrite)) {
          wait = 0;
          clusterToWrite->dump_tmp(creator->data->tmpfname);
          clusterToWrite->close();
          continue;
        }
        wait += 10;
      }
      return nullptr;
    }

    Creator::Creator(bool verbose)
      : verbose(verbose)
    {}

    Creator::~Creator() = default;

    void Creator::startZimCreation(const std::string& fname)
    {
      data = std::unique_ptr<CreatorData>(new CreatorData(fname, verbose, withIndex, indexingLanguage));
      data->setMinChunkSize(minChunkSize);

      for(unsigned i=0; i<compressionThreads; i++)
      {
        pthread_t thread;
        pthread_create(&thread, NULL, clusterWriter, this);
        pthread_detach(thread);
        data->runningWriters.push_back(thread);
      }
    }

    void Creator::addArticle(const Article& article)
    {
      auto dirent = data->createDirentFromArticle(&article);
      data->addDirent(dirent, &article);
      data->nbArticles++;
      if (article.isRedirect()) {
        data->nbRedirectArticles++;
      } else {
        if (article.shouldCompress())
          data->nbCompArticles++;
        else
          data->nbUnCompArticles++;
        if (!article.getFilename().empty())
          data->nbFileArticles++;
        if (article.shouldIndex())
          data->nbIndexArticles++;
      }
      if (verbose && data->nbArticles%1000 == 0){
        double seconds = difftime(time(NULL),data->start_time);
        std::cout << "T:" << (int)seconds
                  << "; A:" << data->nbArticles
                  << "; RA:" << data->nbRedirectArticles
                  << "; CA:" << data->nbCompArticles
                  << "; UA:" << data->nbUnCompArticles
                  << "; FA:" << data->nbFileArticles
                  << "; IA:" << data->nbIndexArticles
                  << "; C:" << data->nbClusters
                  << "; CC:" << data->nbCompClusters
                  << "; UC:" << data->nbUnCompClusters
                  << "; WC:" << data->clustersToWrite.size()
                  << std::endl;
      }

#if defined(ENABLE_XAPIAN)
      if(withIndex && article.shouldIndex()) {
        data->indexer->index(&article);
      }
#endif
    }

    void Creator::finishZimCreation()
    {
      if (verbose) {
        double seconds = difftime(time(NULL),data->start_time);
        std::cout << "T:" << (int)seconds
                  << "; A:" << data->nbArticles
                  << "; RA:" << data->nbRedirectArticles
                  << "; CA:" << data->nbCompArticles
                  << "; UA:" << data->nbUnCompArticles
                  << "; FA:" << data->nbFileArticles
                  << "; IA:" << data->nbIndexArticles
                  << "; C:" << data->nbClusters
                  << "; CC:" << data->nbCompClusters
                  << "; UC:" << data->nbUnCompClusters
                  << "; WC:" << data->clustersToWrite.size()
                  << std::endl;
      }

#if defined(ENABLE_XAPIAN)
      if (withIndex) {
          data->indexer->indexingPostlude();
          microsleep(100);
          auto article = data->indexer->getMetaArticle();
          auto dirent = data->createDirentFromArticle(article);
          data->addDirent(dirent, article);
          delete article;
      }
#endif

      // When we've seen all articles, write any remaining clusters.
      if (data->compCluster->count())
        data->closeCluster(true);

      if (data->uncompCluster->count())
        data->closeCluster(false);

      // wait all cluster writing has been done
      unsigned int wait = 0;
      do {
        microsleep(wait);
        wait += 10;
      } while(!data->clustersToWrite.isEmpty());

      // Be sure that all cluster are closed
      wait = 0;
      bool closed = true;
      do {
        closed = true;
        microsleep(wait);
        wait += 10;
        for(auto cluster: data->clustersList) {
          if (!cluster->isClosed()) {
            closed = false;
            break;
          }
        }
      } while(!closed);

// [FIXME] pthread_cancel is not defined in android NDK.
// As we don't create zim on android platform, 
// let's simply skip this code to still allow
// compilation of libzim on android.
#if !defined(__ANDROID__)
      for(auto& thread: data->runningWriters)
      {
        pthread_cancel(thread);
      }
#endif

      TINFO("Generate cluster offsets");
      data->generateClustersOffsets();

      TINFO("ResolveRedirectIndexes");
      data->resolveRedirectIndexes();

      TINFO("Set article indexes");
      data->setArticleIndexes();

      TINFO("Resolve mimetype");
      data->resolveMimeTypes();

      TINFO("create title index");
      data->createTitleIndex();
      TINFO(data->dirents.size() << " title index created");
      TINFO(data->clusterOffsets.size() << " clusters created");

      TINFO("fill header");
      Fileheader header;
      fillHeader(&header);

      TINFO("write zimfile :");
      write(header, data->basename + ".zim.tmp");
      zim::DEFAULTFS::rename(data->basename + ".zim.tmp", data->basename + ".zim");

      TINFO("finish");
    }

    void Creator::fillHeader(Fileheader* header)
    {
      auto mainUrl = getMainUrl();
      auto layoutUrl = getLayoutUrl();

      log_debug("main url=" << mainUrl << " layout url=" << layoutUrl);

      if (data->isExtended) {
        header->setMajorVersion(Fileheader::zimExtendedMajorVersion);
      } else {
        header->setMajorVersion(Fileheader::zimClassicMajorVersion);
      }
      header->setMinorVersion(Fileheader::zimMinorVersion);
      header->setMainPage(std::numeric_limits<article_index_type>::max());
      header->setLayoutPage(std::numeric_limits<article_index_type>::max());

      if (!mainUrl.empty() || !layoutUrl.empty())
      {
        for (auto& dirent: data->dirents)
        {
          if (mainUrl == dirent->getFullUrl())
          {
            log_debug("main idx=" << dirent->getIdx());
            header->setMainPage(article_index_type(dirent->getIdx()));
          }

          if (layoutUrl == dirent->getFullUrl())
          {
            log_debug("layout idx=" << dirent->getIdx());
            header->setLayoutPage(article_index_type(dirent->getIdx()));
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

    void Creator::write(const Fileheader& header, const std::string& fname) const
    {
      std::ofstream zimfile(fname);
      Md5stream md5;
      Tee out(zimfile, md5);

      TINFO(" write header");
      out << header;

      log_debug("after writing header - pos=" << zimfile.tellp());

      TINFO(" write mimetype list");
      for(auto& mimeType: data->mimeTypesList)
      {
        out << mimeType << '\0';
      }

      out << '\0';

      TINFO(" write url prt list");
      offset_t off(header.getTitleIdxPos() + data->titleIdxSize().v);
      for (auto& dirent: data->dirents)
      {
        char tmp_buff[sizeof(offset_type)];
        toLittleEndian(off.v, tmp_buff);
        out.write(tmp_buff, sizeof(offset_type));
        off += dirent->getDirentSize();
      }

      log_debug("after writing direntPtr - pos=" << out.tellp());

      TINFO(" write title index");
      for (Dirent* dirent: data->titleIdx)
      {
        char tmp_buff[sizeof(article_index_type)];
        toLittleEndian(dirent->getIdx().v, tmp_buff);
        out.write(tmp_buff, sizeof(article_index_type));
      }

      log_debug("after writing fileIdxList - pos=" << out.tellp());

      TINFO(" write directory entries");
      for (Dirent* dirent: data->dirents)
      {
        out << *dirent;
        log_debug("write " << dirent->getTitle() << " dirent.size()=" << dirent->getDirentSize() << " pos=" << out.tellp());
      }

      log_debug("after writing dirents - pos=" << out.tellp());

      TINFO(" write cluster offset list");
      off += data->clusterPtrSize();
      for (auto clusterOffset : data->clusterOffsets)
      {
        offset_t o(off + clusterOffset);
        char tmp_buff[sizeof(offset_type)];
        toLittleEndian(o.v, tmp_buff);
        out.write(tmp_buff, sizeof(offset_type));
      }

      log_debug("after writing clusterOffsets - pos=" << out.tellp());

      TINFO(" write cluster data");
      if (!data->isEmpty)
      {
        int nb_written_clusters = 0;
        for(auto& cluster: data->clustersList)
        {
          ASSERT(cluster->isClosed(), ==, true);
          cluster->write_final(out);
          if (((++nb_written_clusters) % 1000) == 0) {
            TINFO(nb_written_clusters << "/" << data->clustersList.size());
          }
        }
      }
      else
        log_warn("no data found");

      if (!out)
        throw std::runtime_error("failed to write zimfile");

      log_debug("after writing clusterData - pos=" << out.tellp());

      TINFO(" write checksum");
      unsigned char digest[16];
      md5.getDigest(digest);
      zimfile.write(reinterpret_cast<const char*>(digest), 16);
    }

    CreatorData::CreatorData(const std::string& fname,
                                   bool verbose,
                                   bool withIndex,
                                   std::string language)
      : withIndex(withIndex),
        indexingLanguage(language),
        verbose(verbose),
        nbArticles(0),
        nbRedirectArticles(0),
        nbCompArticles(0),
	nbUnCompArticles(0),
	nbFileArticles(0),
	nbIndexArticles(0),
	nbClusters(0),
	nbCompClusters(0),
	nbUnCompClusters(0),
        start_time(time(NULL))
    {
      basename =  (fname.size() > 4 && fname.compare(fname.size() - 4, 4, ".zim") == 0)
                        ? fname.substr(0, fname.size() - 4)
                        : fname;
      tmpfname = basename + ".tmp";
      if(!DEFAULTFS::makeDirectory(tmpfname)) {
        throw std::runtime_error(
          std::string("failed to create temporary directory ")
        + tmpfname);
      }

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

    CreatorData::~CreatorData()
    {
      if (compCluster)
        delete compCluster;
      if (uncompCluster)
        delete uncompCluster;
      for(auto& cluster: clustersList) {
        delete cluster;
      }
#ifndef _WIN32
//[TODO] Implement remove for windows
      DEFAULTFS::remove(tmpfname);
#endif
#if defined(ENABLE_XAPIAN)
      if (indexer)
        delete indexer;
#endif
    }

    void CreatorData::addDirent(Dirent* dirent, const Article* article)
    {
      auto ret = dirents.insert(dirent);
      if (!ret.second) {
        Dirent* existing = *ret.first;
        if (existing->isRedirect() && !dirent->isRedirect()) {
          unresolvedRedirectDirents.erase(existing);
          dirents.erase(ret.first);
          dirents.insert(dirent);
        } else {
          std::cerr << "Impossible to add " << dirent->getFullUrl().getLongUrl() << std::endl;
          std::cerr << "  dirent's title to add is : " << dirent->getTitle() << std::endl;
          std::cerr << "  existing dirent's title is : " << existing->getTitle() << std::endl;
          return;
        }
      };

      // If this is a redirect, we're done: there's no blob to add.
      if (dirent->isRedirect())
      {
        unresolvedRedirectDirents.insert(dirent);
        return;
      }

      // Add blob data to compressed or uncompressed cluster.
      auto articleSize = article->getSize();
      if (articleSize > 0)
      {
        isEmpty = false;
      }

      Cluster *cluster;
      if (article->shouldCompress())
      {
        cluster = compCluster;
      }
      else
      {
        cluster = uncompCluster;
      }

      // If cluster will be too large, write it to dis, and open a new
      // one for the content.
      if ( cluster->count()
        && cluster->size().v+articleSize >= minChunkSize * 1024
         )
      {
        log_info("cluster with " << cluster->count() << " articles, " <<
                 cluster->size() << " bytes; current title \"" <<
                 dirent->getTitle() << '\"');
        cluster = closeCluster(article->shouldCompress());
      }

      dirent->setCluster(cluster);
      cluster->addArticle(article);
    }

    Dirent* CreatorData::createDirentFromArticle(const Article* article)
    {
      auto dirent = pool.getDirent();
      dirent->setUrl(article->getUrl());
      dirent->setTitle(article->getTitle());

      log_debug("article " << dirent->getLongUrl() << " fetched");

      if (article->isRedirect())
      {
        dirent->setRedirect(nullptr);
        dirent->setRedirectUrl(article->getRedirectUrl());
        log_debug("is redirect to " << dirent->getRedirectUrl());
      }
      else if (article->isLinktarget())
      {
        dirent->setLinktarget();
      }
      else if (article->isDeleted())
      {
        dirent->setDeleted();
      }
      else
      {
        auto mimetype = article->getMimeType();
        if (mimetype.empty()) {
          std::cerr << "Warning, " << article->getUrl().getLongUrl() << " have empty mimetype." << std::endl;
          mimetype = "application/octet-stream";
        }
        dirent->setMimeType(getMimeTypeIdx(mimetype));
        log_debug("is article; mimetype " << dirent->getMimeType());
      }
      return dirent;
    }

    Cluster* CreatorData::closeCluster(bool compressed)
    {
      Cluster *cluster;
      nbClusters++;
      if (compressed )
      {
        cluster = compCluster;
        nbCompClusters++;
      } else {
        cluster = uncompCluster;
        nbUnCompClusters++;
      }
      cluster->setClusterIndex(cluster_index_t(clustersList.size()));
      clustersList.push_back(cluster);
      clustersToWrite.pushToQueue(cluster);

      log_debug("cluster written");
      if (cluster->is_extended() )
        isExtended = true;
      if (compressed)
      {
        cluster = compCluster = new Cluster(compression);
      } else {
        cluster = uncompCluster = new Cluster(zimcompNone);
      }
      return cluster;
    }

    void CreatorData::generateClustersOffsets()
    {
      clustersSize = zsize_t(0);
      for(auto& cluster: clustersList)
      {
        clusterOffsets.push_back(offset_t(clustersSize.v));
        clustersSize += cluster->getFinalSize();
      }
    }

    void CreatorData::setArticleIndexes()
    {
      // set index
      INFO("set index");
      article_index_t idx(0);
      for (auto& dirent: dirents) {
        dirent->setIdx(idx);
        idx += 1;
      }
    }

    void CreatorData::resolveRedirectIndexes()
    {
      // translate redirect aid to index
      INFO("Resolve redirect");
      for (auto dirent: unresolvedRedirectDirents)
      {
        Dirent tmpDirent(dirent->getRedirectUrl());
        auto target_pos = dirents.find(&tmpDirent);
        if(target_pos == dirents.end()) {
          INFO("Invalid redirection " << dirent->getFullUrl().getLongUrl() << " redirecting to (missing) " << dirent->getRedirectUrl().getLongUrl());
          dirents.erase(dirent);
        } else  {
          dirent->setRedirect(*target_pos);
        }
      }
    }

    void CreatorData::createTitleIndex()
    {
      titleIdx.clear();
      for (auto dirent: dirents)
        titleIdx.insert(dirent);
    }

    void CreatorData::resolveMimeTypes()
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
        if (dirent->isArticle())
          dirent->setMimeType(mapping[dirent->getMimeType()]);
      }
    }

    uint16_t CreatorData::getMimeTypeIdx(const std::string& mimeType)
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

    const std::string& CreatorData::getMimeType(uint16_t mimeTypeIdx) const
    {
      auto it = rmimeTypesMap.find(mimeTypeIdx);
      if (it == rmimeTypesMap.end())
        throw std::runtime_error("mime type index not found");
      return it->second;
    }

    zsize_t CreatorData::mimeListSize() const
    {
      size_type ret = 1;
      for (auto& rmimeType: rmimeTypesMap)
        ret += (rmimeType.second.size() + 1);
      return zsize_t(ret);
    }

    zsize_t CreatorData::indexSize() const
    {
      size_type s = 0;

      for (auto& dirent: dirents)
        s += dirent->getDirentSize();

      return zsize_t(s);
    }

  }
}
