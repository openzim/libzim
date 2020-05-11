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
#include "workers.h"
#include <zim/blob.h>
#include <zim/writer/creator.h>
#include "../endian_tools.h"
#include <algorithm>
#include <fstream>
#include "../md5.h"

#if defined(ENABLE_XAPIAN)
  #include "xapianIndexer.h"
#endif

#ifdef _WIN32
# include <io.h>
#else
# include <unistd.h>
# define _write(fd, addr, size) if(::write((fd), (addr), (size)) != (ssize_t)(size)) \
{throw std::runtime_error("Error writing");}
#endif

#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <limits>
#include <stdexcept>
#include <sstream>
#include <ctime>
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

#define CLUSTER_BASE_OFFSET 1024

namespace zim
{
  namespace writer
  {
    Creator::Creator(bool verbose)
      : verbose(verbose)
    {}

    Creator::~Creator() = default;

    void Creator::startZimCreation(const std::string& fname)
    {
      data = std::unique_ptr<CreatorData>(new CreatorData(fname, verbose, withIndex, indexingLanguage));
      data->setMinChunkSize(minChunkSize);

      for(unsigned i=0; i<nbWorkerThreads; i++)
      {
        pthread_t thread;
        pthread_create(&thread, NULL, taskRunner, this->data.get());
        data->workerThreads.push_back(thread);
      }

      pthread_create(&data->writerThread, NULL, clusterWriter, this->data.get());
    }

    void Creator::addArticle(std::shared_ptr<Article> article)
    {
      auto dirent = data->createDirentFromArticle(article.get());
      data->addDirent(dirent, article.get());
      data->nbArticles++;
      if (article->isRedirect()) {
        data->nbRedirectArticles++;
      } else {
        if (article->shouldCompress())
          data->nbCompArticles++;
        else
          data->nbUnCompArticles++;
        if (!article->getFilename().empty())
          data->nbFileArticles++;
        if (article->shouldIndex())
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
                  << "; WC:" << data->taskList.size()
                  << std::endl;
      }

#if defined(ENABLE_XAPIAN)
      if (article->shouldIndex()) {
        data->titleIndexer.index(article.get());
        if(withIndex && !article->isRedirect()) {
          data->taskList.pushToQueue(new IndexTask(article));
        }
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
                  << "; WC:" << data->taskList.size()
                  << std::endl;
      }

      // We need to wait that all indexation task has been done before closing the
      // xapian database and add it to zim.
      unsigned int wait = 0;
      do {
        microsleep(wait);
        wait += 10;
      } while(IndexTask::waiting_task.load() > 0);

#if defined(ENABLE_XAPIAN)
      {
        data->titleIndexer.indexingPostlude();
        auto article = data->titleIndexer.getMetaArticle();
        auto dirent = data->createDirentFromArticle(article);
        data->addDirent(dirent, article);
        delete article;
      }
      if (withIndex) {
        wait = 0;
        do {
          microsleep(wait);
          wait += 10;
        } while(IndexTask::waiting_task.load() > 0);

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

      TINFO("Waiting for workers");
      // wait all cluster compression has been done
      wait = 0;
      do {
        microsleep(wait);
        wait += 10;
      } while(ClusterTask::waiting_task.load() > 0);

      // Quit all workerThreads
      for (auto i=0U; i< nbWorkerThreads; i++) {
        data->taskList.pushToQueue(nullptr);
      }
      for(auto& thread: data->workerThreads) {
        pthread_join(thread, nullptr);
      }

      // Wait for writerThread to finish.
      data->clusterToWrite.pushToQueue(nullptr);
      pthread_join(data->writerThread, nullptr);

      TINFO("ResolveRedirectIndexes");
      data->resolveRedirectIndexes();

      TINFO("Set article indexes");
      data->setArticleIndexes();

      TINFO("Resolve mimetype");
      data->resolveMimeTypes();

      TINFO("create title index");
      data->createTitleIndex();
      TINFO(data->dirents.size() << " title index created");
      TINFO(data->clustersList.size() << " clusters created");

      TINFO("write zimfile :");
      write();
      ::close(data->out_fd);

      TINFO("finish");
    }

    void Creator::fillHeader(Fileheader* header) const
    {
      auto mainUrl = getMainUrl();
      auto layoutUrl = getLayoutUrl();

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
            header->setMainPage(article_index_type(dirent->getIdx()));
          }

          if (layoutUrl == dirent->getFullUrl())
          {
            header->setLayoutPage(article_index_type(dirent->getIdx()));
          }
        }
      }

      header->setUuid( getUuid() );
      header->setArticleCount( data->dirents.size() );

      header->setMimeListPos( Fileheader::size );

      header->setClusterCount( data->clustersList.size() );
    }

    void Creator::write() const
    {
      Fileheader header;
      fillHeader(&header);

      int out_fd = data->out_fd;

      lseek(out_fd, header.getMimeListPos(), SEEK_SET);
      TINFO(" write mimetype list");
      for(auto& mimeType: data->mimeTypesList)
      {
        _write(out_fd, mimeType.c_str(), mimeType.size()+1);
      }

      _write(out_fd, "", 1);

      ASSERT(lseek(out_fd, 0, SEEK_CUR), <, CLUSTER_BASE_OFFSET);

      TINFO(" write directory entries");
      lseek(out_fd, 0, SEEK_END);
      for (Dirent* dirent: data->dirents)
      {
        dirent->setOffset(offset_t(lseek(out_fd, 0, SEEK_CUR)));
        dirent->write(out_fd);
      }

      TINFO(" write url prt list");
      header.setUrlPtrPos(lseek(out_fd, 0, SEEK_CUR));
      for (auto& dirent: data->dirents)
      {
        char tmp_buff[sizeof(offset_type)];
        toLittleEndian(dirent->getOffset(), tmp_buff);
        _write(out_fd, tmp_buff, sizeof(offset_type));
      }

      TINFO(" write title index");
      header.setTitleIdxPos(lseek(out_fd, 0, SEEK_CUR));
      for (Dirent* dirent: data->titleIdx)
      {
        char tmp_buff[sizeof(article_index_type)];
        toLittleEndian(dirent->getIdx().v, tmp_buff);
        _write(out_fd, tmp_buff, sizeof(article_index_type));
      }

      TINFO(" write cluster offset list");
      header.setClusterPtrPos(lseek(out_fd, 0, SEEK_CUR));
      for (auto cluster : data->clustersList)
      {
        char tmp_buff[sizeof(offset_type)];
        toLittleEndian(cluster->getOffset(), tmp_buff);
        _write(out_fd, tmp_buff, sizeof(offset_type));
      }

      header.setChecksumPos(lseek(out_fd, 0, SEEK_CUR));

      TINFO(" write header");
      lseek(out_fd, 0, SEEK_SET);
      header.write(out_fd);

      TINFO(" write checksum");
      struct zim_MD5_CTX md5ctx;
      unsigned char batch_read[1024+1];
      lseek(out_fd, 0, SEEK_SET);
      zim_MD5Init(&md5ctx);
      while (true) {
         auto r = read(out_fd, batch_read, 1024);
         if (r == -1) {
           perror("Cannot read");
           throw std::runtime_error("oups");
         }
         if (r == 0)
           break;
         batch_read[r] = 0;
         zim_MD5Update(&md5ctx, batch_read, r);
      }
      unsigned char digest[16];
      zim_MD5Final(digest, &md5ctx);
      _write(out_fd, reinterpret_cast<const char*>(digest), 16);
    }

    CreatorData::CreatorData(const std::string& fname,
                                   bool verbose,
                                   bool withIndex,
                                   std::string language)
      : withIndex(withIndex),
        indexingLanguage(language),
#if defined(ENABLE_XAPIAN)
        titleIndexer(language, IndexingMode::TITLE, true),
#endif
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
      auto zim_name = basename + ".zim";
#ifdef _WIN32
int mode =  _S_IREAD | _S_IWRITE;
#else
      mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
#endif
      out_fd = open(zim_name.c_str(), O_RDWR|O_CREAT|O_TRUNC, mode);
      if (out_fd == -1){
        perror(nullptr);
        std::ostringstream ss;
        ss << "Cannot create file " << zim_name;
        throw std::runtime_error(ss.str());
      }
      if(lseek(out_fd, CLUSTER_BASE_OFFSET, SEEK_SET) != CLUSTER_BASE_OFFSET) {
        close(out_fd);
        perror(nullptr);
        throw std::runtime_error("Impossible to seek in file");
      }

      // We keep both a "compressed cluster" and an "uncompressed cluster"
      // because we don't know which one will fill up first.  We also need
      // to track the dirents currently in each, so we can fix up the
      // cluster index if the other one ends up written first.
      compCluster = new Cluster(compression);
      uncompCluster = new Cluster(zimcompNone);

#if defined(ENABLE_XAPIAN)
      titleIndexer.indexingPrelude(tmpfname+"_title.idx");
      if (withIndex) {
          indexer = new XapianIndexer(indexingLanguage, IndexingMode::FULL, true);
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

      if (article->isRedirect())
      {
        dirent->setRedirect(nullptr);
        dirent->setRedirectUrl(article->getRedirectUrl());
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
      taskList.pushToQueue(new ClusterTask(cluster));
      clusterToWrite.pushToQueue(cluster);

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
  }
}
