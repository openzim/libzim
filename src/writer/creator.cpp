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

#include <zim/writer/creator.h>

#include "config.h"

#include "creatordata.h"
#include "cluster.h"
#include "debug.h"
#include "workers.h"
#include "clusterWorker.h"
#include <zim/blob.h>
#include <zim/writer/contentProvider.h>
#include "../endian_tools.h"
#include <algorithm>
#include <fstream>
#include "../md5.h"

#if defined(ENABLE_XAPIAN)
# include "xapianHandler.h"
#endif

#ifdef _WIN32
# include <io.h>
# include <fcntl.h>
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
    if (m_verbose) { \
        double seconds = difftime(time(NULL), data->start_time); \
        std::cout << "T:" << (int)(seconds) \
                  << "; " << e << std::endl; \
    }

#define TPROGRESS() \
    if (m_verbose ) { \
        double seconds = difftime(time(NULL),data->start_time);  \
        std::cout << "T:" << (int)seconds \
                  << "; A:" << data->dirents.size() \
                  << "; RA:" << data->nbRedirectItems \
                  << "; CA:" << data->nbCompItems \
                  << "; UA:" << data->nbUnCompItems \
                  << "; C:" << data->nbClusters \
                  << "; CC:" << data->nbCompClusters \
                  << "; UC:" << data->nbUnCompClusters \
                  << "; WC:" << data->taskList.size() \
                  << std::endl; \
    }


#define CLUSTER_BASE_OFFSET 1024

namespace zim
{
  namespace writer
  {
    Creator::Creator() = default;
    Creator::~Creator() = default;

    Creator& Creator::configVerbose(bool verbose)
    {
      m_verbose = verbose;
      return *this;
    }

    Creator& Creator::configCompression(CompressionType comptype)
    {
      switch(comptype) {
        case zim::zimcompBzip2:
        case zim::zimcompZip:
          throw std::runtime_error("Compression method not enabled in this library");

        case zimcompLzma:
          std::cerr << "WARNING: LZMA compression method is deprecated."
                    << " Support for it will be dropped from libzim soon."
                    << std::endl;
          break;

        default:
          break;
      }
      m_compression = comptype;
      return *this;
    }

    Creator& Creator::configMinClusterSize(zim::size_type size)
    {
      m_minClusterSize = size;
      return *this;
    }

    Creator& Creator::configIndexing(bool indexing, std::string language)
    {
      m_withIndex = indexing;
      m_indexingLanguage = language;
      return *this;
    }

    Creator& Creator::configNbWorkers(unsigned nbWorkers)
    {
      m_nbWorkers = nbWorkers;
      return *this;
    }

    void Creator::startZimCreation(const std::string& filepath)
    {
      data = std::unique_ptr<CreatorData>(
        new CreatorData(filepath, m_verbose, m_withIndex, m_indexingLanguage, m_compression)
      );
      data->setMinChunkSize(m_minClusterSize);

      for(unsigned i=0; i<m_nbWorkers; i++)
      {
        std::thread thread(taskRunner, this->data.get());
        data->workerThreads.push_back(std::move(thread));
      }

      data->writerThread = std::thread(clusterWriter, this->data.get());
    }

    void Creator::addItem(std::shared_ptr<Item> item)
    {
      checkError();
      auto hints = item->getHints();

      bool compressContent;
      try {
        compressContent = bool(hints.at(COMPRESS));
      } catch(std::out_of_range&) {
        compressContent = isCompressibleMimetype(item->getMimeType());
      }

      auto dirent = data->createItemDirent(item.get());
      data->addItemData(dirent, item->getContentProvider(), compressContent);
      data->handle(dirent, item);

      if (data->dirents.size()%1000 == 0) {
        TPROGRESS();
      }

    }

    void Creator::addMetadata(const std::string& name, const std::string& content, const std::string& mimetype)
    {
      checkError();
      auto provider = std::unique_ptr<ContentProvider>(new StringProvider(content));
      addMetadata(name, std::move(provider), mimetype);
    }

    void Creator::addMetadata(const std::string& name, std::unique_ptr<ContentProvider> provider, const std::string& mimetype)
    {
      checkError();
      auto compressContent = isCompressibleMimetype(mimetype);
      auto dirent = data->createDirent('M', name, mimetype, "");
      data->addItemData(dirent, std::move(provider), compressContent);
      data->handle(dirent);
    }

    void Creator::addRedirection(const std::string& path, const std::string& title, const std::string& targetPath, const Hints& hints)
    {
      checkError();
      auto dirent = data->createRedirectDirent('C', path, title, 'C', targetPath);
      if (data->dirents.size()%1000 == 0){
        TPROGRESS();
      }

      data->handle(dirent, hints);
    }

    void Creator::finishZimCreation()
    {
      checkError();
      // Create mandatory entries
      if (!m_faviconPath.empty()) {
        auto dirent = data->createRedirectDirent('W', "favicon", "", 'C', m_faviconPath);
        data->handle(dirent);
      }

      // Create a redirection for the mainPage.
      // We need to keep the created dirent to set the fileheader.
      // Dirent doesn't have to be deleted.
      if (!m_mainPath.empty()) {
        data->mainPageDirent = data->createRedirectDirent('W', "mainPage", "", 'C', m_mainPath);
        data->handle(data->mainPageDirent);
      }

      TPROGRESS();

      // mp_titleListingHandler is a special case, it have to handle all dirents (including itself)
      for(auto& handler:data->m_direntHandlers) {
        // This silently create all the needed dirents
        auto dirent = handler->getDirent();
        data->mp_titleListingHandler->handle(dirent, Hints());
      }

      // Now we have all the dirents (but not the data), we must correctly set/fix the dirents
      // before we ask data to the handlers
      TINFO("ResolveRedirectIndexes");
      data->resolveRedirectIndexes();

      TINFO("Set entry indexes");
      data->setEntryIndexes();

      TINFO("Resolve mimetype");
      data->resolveMimeTypes();

      // We can now stop the direntHandlers, and get their content
      for(auto& handler:data->m_direntHandlers) {
        handler->stop();
        auto dirent = handler->getDirent();
        auto provider = handler->getContentProvider();
        data->addItemData(dirent, std::move(provider), false);
        if (handler == data->mp_titleListingHandler) {
          // We have to get the offset of the titleList in the cluster before
          // we close the cluster. Once the cluster is close, the offset information is dropped.
          data->m_titleListBlobOffset = data->uncompCluster->getBlobOffset(dirent->getBlobNumber());
        }
      }

      // All the data has been added, we can now close all clusters
      if (data->compCluster->count())
        data->closeCluster(true);

      if (data->uncompCluster->count())
        data->closeCluster(false);

      TINFO("Waiting for workers");
      // wait all cluster compression has been done
      unsigned int wait = 0;
      do {
        microsleep(wait);
        wait += 10;
      } while(ClusterTask::waiting_task.load() > 0);

      data->closeThreads();

      TINFO(data->dirents.size() << " title index created");
      TINFO(data->clustersList.size() << " clusters created");

      TINFO("write zimfile :");
      write();
      ::close(data->out_fd);

      TINFO("rename tmpfile to final one.");
      DEFAULTFS::rename(data->tmpFileName, data->zimName);

      TINFO("finish");
    }

    void Creator::fillHeader(Fileheader* header) const
    {
      header->setMainPage(
        data->mainPageDirent
        ? entry_index_type(data->mainPageDirent->getIdx())
        : std::numeric_limits<entry_index_type>::max());
      header->setLayoutPage(std::numeric_limits<entry_index_type>::max());

      header->setUuid( m_uuid );
      header->setArticleCount( data->dirents.size() );

      header->setMimeListPos( Fileheader::size );

      auto cluster = data->mp_titleListingHandler->getDirent()->getCluster();
      header->setTitleIdxPos(
        offset_type(cluster->getOffset() + cluster->getDataOffset() + data->m_titleListBlobOffset));

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

    void Creator::checkError()
    {
      if (data->m_errored) {
        throw std::runtime_error("Creator is in error state");
      }
      std::lock_guard<std::mutex> l(data->m_exceptionLock);
      if (data->m_exceptionSlot) {
        std::cerr << "ERROR Detected" << std::endl;
        data->m_errored = true;
        std::rethrow_exception(data->m_exceptionSlot);
      }
    }

    CreatorData::CreatorData(const std::string& fname,
                                   bool verbose,
                                   bool withIndex,
                                   std::string language,
                                   CompressionType c)
      : mainPageDirent(nullptr),
        m_errored(false),
        compression(c),
        zimName(fname),
        tmpFileName(fname + ".tmp"),
        withIndex(withIndex),
        indexingLanguage(language),
        verbose(verbose),
        nbRedirectItems(0),
        nbCompItems(0),
        nbUnCompItems(0),
        nbClusters(0),
        nbCompClusters(0),
        nbUnCompClusters(0),
        start_time(time(NULL))
    {
#ifdef _WIN32
      int flag = _O_RDWR | _O_CREAT | _O_TRUNC | _O_BINARY;
      int mode =  _S_IREAD | _S_IWRITE;
#else
      int flag = O_RDWR | O_CREAT | O_TRUNC;
      mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
#endif
      out_fd = open(tmpFileName.c_str(), flag, mode);
      if (out_fd == -1){
        perror(nullptr);
        std::ostringstream ss;
        ss << "Cannot create file " << tmpFileName;
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
      auto titleIndexer = std::make_shared<TitleXapianHandler>(this);
      m_direntHandlers.push_back(titleIndexer);
      if (withIndex) {
        auto fulltextIndexer = std::make_shared<FullTextXapianHandler>(this);
        m_direntHandlers.push_back(fulltextIndexer);
      }
#endif

      mp_titleListingHandler = std::make_shared<TitleListingHandler>(this);
      m_direntHandlers.push_back(mp_titleListingHandler);
      m_direntHandlers.push_back(std::make_shared<TitleListingHandlerV1>(this));

      for(auto& handler:m_direntHandlers) {
        handler->start();
      }
    }

    CreatorData::~CreatorData()
    {
      closeThreads();
      if (compCluster)
        delete compCluster;
      if (uncompCluster)
        delete uncompCluster;
      for(auto& cluster: clustersList) {
        delete cluster;
      }
    }

    void CreatorData::closeThreads()
    {
      // Quit all workerThreads
      for (auto i=0U; i< workerThreads.size(); i++) {
        taskList.pushToQueue(nullptr);
      }
      for(auto& thread: workerThreads) {
        if (thread.joinable()) {
          thread.join();
        }
      }

      // Wait for writerThread to finish.
      clusterToWrite.pushToQueue(nullptr);
      if (writerThread.joinable()) {
        writerThread.join();
      }
    }

    void CreatorData::addDirent(Dirent* dirent)
    {
      auto ret = dirents.insert(dirent);
      if (!ret.second) {
        Dirent* existing = *ret.first;
        if (existing->isRedirect() && !dirent->isRedirect()) {
          unresolvedRedirectDirents.erase(existing);
          dirents.erase(ret.first);
          dirents.insert(dirent);
        } else {
          std::cerr << "Impossible to add " << dirent->getNamespace() << "/" << dirent->getPath() << std::endl;
          std::cerr << "  dirent's title to add is : " << dirent->getTitle() << std::endl;
          std::cerr << "  existing dirent's title is : " << existing->getTitle() << std::endl;
          return;
        }
      };

      // If this is a redirect, we're done: there's no blob to add.
      if (dirent->isRedirect())
      {
        unresolvedRedirectDirents.insert(dirent);
        nbRedirectItems++;
        return;
      }
    }

    void CreatorData::addItemData(Dirent* dirent, std::unique_ptr<ContentProvider> provider, bool compressContent)
    {
      // Add blob data to compressed or uncompressed cluster.
      auto itemSize = provider->getSize();
      if (itemSize > 0)
      {
        isEmpty = false;
      }

      auto cluster = compressContent ? compCluster : uncompCluster;

      // If cluster will be too large, write it to dis, and open a new
      // one for the content.
      if ( cluster->count()
        && cluster->size().v+itemSize >= minChunkSize * 1024
         )
      {
        log_info("cluster with " << cluster->count() << " items, " <<
                 cluster->size() << " bytes; current title \"" <<
                 dirent->getTitle() << '\"');
        cluster = closeCluster(compressContent);
      }

      dirent->setCluster(cluster);
      cluster->addContent(std::move(provider));

      if (compressContent) {
        nbCompItems++;
      } else {
        nbUnCompItems++;
      }

    }

    Dirent* CreatorData::createDirent(char ns, const std::string& path, const std::string& mimetype, const std::string& title)
    {
      auto dirent = pool.getDirent();
      dirent->setNamespace(ns);
      dirent->setPath(path);
      dirent->setMimeType(getMimeTypeIdx(mimetype));
      dirent->setTitle(title);
      addDirent(dirent);
      return dirent;
    }

    Dirent* CreatorData::createItemDirent(const Item* item)
    {
      auto path = item->getPath();
      auto mimetype = item->getMimeType();
      if (mimetype.empty()) {
        std::cerr << "Warning, " << item->getPath() << " have empty mimetype." << std::endl;
        mimetype = "application/octet-stream";
      }
      return createDirent('C', item->getPath(), mimetype, item->getTitle());
    }

    Dirent* CreatorData::createRedirectDirent(char ns, const std::string& path, const std::string& title, char targetNs, const std::string& targetPath)
    {
      auto dirent = pool.getDirent();
      dirent->setNamespace(ns);
      dirent->setPath(path);
      dirent->setTitle(title);
      dirent->setRedirectNs(targetNs);
      dirent->setRedirectPath(targetPath);
      dirent->setRedirect(nullptr);
      addDirent(dirent);
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

      if (compressed)
      {
        cluster = compCluster = new Cluster(compression);
      } else {
        cluster = uncompCluster = new Cluster(zimcompNone);
      }
      return cluster;
    }

    void CreatorData::setEntryIndexes()
    {
      // set index
      INFO("set index");
      entry_index_t idx(0);
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
        Dirent tmpDirent(dirent->getRedirectNs(), dirent->getRedirectPath());
        auto target_pos = dirents.find(&tmpDirent);
        if(target_pos == dirents.end()) {
          INFO("Invalid redirection "
              << dirent->getNamespace() << '/' << dirent->getPath()
              << " redirecting to (missing) "
              << dirent->getRedirectNs() << '/' << dirent->getRedirectPath());
          dirents.erase(dirent);
          dirent->markRemoved();
          if (dirent == mainPageDirent) {
            mainPageDirent = nullptr;
          }
        } else  {
          dirent->setRedirect(*target_pos);
        }
      }
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
        if (dirent->isItem())
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
