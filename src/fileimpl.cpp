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

#include "fileimpl.h"
#include <zim/error.h>
#include "_dirent.h"
#include "file_compound.h"
#include "file_reader.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <errno.h>
#include <cstring>
#include <fstream>
#include "config.h"
#include "log.h"
#include "envvalue.h"
#include "md5.h"

log_define("zim.file.impl")

namespace zim
{
  //////////////////////////////////////////////////////////////////////
  // FileImpl
  //
  FileImpl::FileImpl(const std::string& fname)
    : zimFile(new FileCompound(fname)),
      zimReader(new FileReader(zimFile)),
      bufferDirentZone(256),
      bufferDirentLock(PTHREAD_MUTEX_INITIALIZER),
      filename(fname),
      direntCache(envValue("ZIM_DIRENTCACHE", DIRENT_CACHE_SIZE)),
      direntCacheLock(PTHREAD_MUTEX_INITIALIZER),
      clusterCache(envValue("ZIM_CLUSTERCACHE", CLUSTER_CACHE_SIZE)),
      clusterCacheLock(PTHREAD_MUTEX_INITIALIZER),
      cacheUncompressedCluster(envValue("ZIM_CACHEUNCOMPRESSEDCLUSTER", false)),
      namespaceBeginLock(PTHREAD_MUTEX_INITIALIZER),
      namespaceEndLock(PTHREAD_MUTEX_INITIALIZER)
  {
    log_trace("read file \"" << fname << '"');

    if (zimFile->fail())
      throw ZimFileFormatError(std::string("can't open zim-file \"") + fname + '"');

    filename = fname;

    // read header
    if (size_type(zimReader->size()) < Fileheader::size) {
      throw ZimFileFormatError("zim-file is too small to contain a header");
    }
    try {
      header.read(zimReader->get_buffer(offset_t(0), zsize_t(Fileheader::size)));
    } catch (ZimFileFormatError& e) {
      throw e;
    } catch (...) {
      throw ZimFileFormatError("error reading zim-file header.");
    }

    // urlPtrOffsetReader
    zsize_t size(header.getArticleCount() * 8);
    if (!zimReader->can_read(offset_t(header.getUrlPtrPos()), size)) {
      throw ZimFileFormatError("Reading out of zim file.");
    }
#ifdef ENABLE_USE_BUFFER_HEADER
    urlPtrOffsetReader = std::unique_ptr<Reader>(new BufferReader(
	zimReader->get_buffer(offset_t(header.getUrlPtrPos()), size)));
#else
    urlPtrOffsetReader = zimReader->sub_reader(offset_t(header.getUrlPtrPos()), size);
#endif

    // Create titleIndexBuffer
    size = zsize_t(header.getArticleCount() * 4);
    if (!zimReader->can_read(offset_t(header.getTitleIdxPos()), size)) {
      throw ZimFileFormatError("Reading out of zim file.");
    }
#ifdef ENABLE_USE_BUFFER_HEADER
    titleIndexReader = std::unique_ptr<Reader>(new BufferReader(
        zimReader->get_buffer(offset_t(header.getTitleIdxPos()), size)));
#else
    titleIndexReader = zimReader->sub_reader(offset_t(header.getTitleIdxPos()), size);
#endif

    // clusterOffsetBuffer
    size = zsize_t(header.getClusterCount() * 8);
    if (!zimReader->can_read(offset_t(header.getClusterPtrPos()), size)) {
      throw ZimFileFormatError("Reading out of zim file.");
    }
#ifdef ENABLE_USE_BUFFER_HEADER
    clusterOffsetReader = std::unique_ptr<Reader>(new BufferReader(
        zimReader->get_buffer(offset_t(header.getClusterPtrPos()), size)));
#else
    clusterOffsetReader = zimReader->sub_reader(offset_t(header.getClusterPtrPos()), size);
#endif

    if (!getCountClusters())
      log_warn("no clusters found");
    else
    {
      offset_t lastOffset = getClusterOffset(cluster_index_t(cluster_index_type(getCountClusters()) - 1));
      log_debug("last offset=" << lastOffset.v << " file size=" << zimFile->fsize().v);
      if (lastOffset.v > zimFile->fsize().v)
      {
        log_fatal("last offset (" << lastOffset << ") larger than file size (" << zimFile->fsize() << ')');
        throw ZimFileFormatError("last cluster offset larger than file size; file corrupt");
      }
    }

    if (header.hasChecksum() && header.getChecksumPos() != (zimFile->fsize().v-16) ) {
      throw ZimFileFormatError("Checksum position is not valid");
    }

    // read mime types
    // libzim write zims files two ways :
    // - The old way by putting the urlPtrPos just after the mimetype.
    // - The new way by putting the urlPtrPos at the end of the zim files.
    //   In this case, the cluster data are always at 1024 bytes offset and we know that
    //   mimetype list is before this.
    // 1024 seems to be a good maximum size for the mimetype list, even for the "old" way.
    auto endMimeList = std::min(header.getUrlPtrPos(), static_cast<zim::offset_type>(1024));
    size = zsize_t(endMimeList - header.getMimeListPos());
    auto buffer = zimReader->get_buffer(offset_t(header.getMimeListPos()), size);
    offset_t current = offset_t(0);
    while (current.v < size.v)
    {
      offset_type len = strlen(buffer->data(current));

      if (len == 0) {
        break;
      }

      if (current.v + len >= size.v) {
       throw(ZimFileFormatError("Error getting mimelists."));
      }

      std::string mimeType(buffer->data(current), len);
      mimeTypes.push_back(mimeType);

      current += (len + 1);
    }
  }


  std::pair<bool, entry_index_t> FileImpl::findx(char ns, const std::string& url)
  {
    log_debug("find article by url " << ns << " \"" << url << "\",  in file \"" << getFilename() << '"');

    entry_index_type l = entry_index_type(getNamespaceBeginOffset(ns));
    entry_index_type u = entry_index_type(getNamespaceEndOffset(ns));

    if (l == u)
    {
      log_debug("namespace " << ns << " not found");
      return std::pair<bool, entry_index_t>(false, entry_index_t(0));
    }

    unsigned itcount = 0;
    while (u - l > 1)
    {
      ++itcount;
      entry_index_type p = l + (u - l) / 2;
      auto d = getDirent(entry_index_t(p));

      int c = ns < d->getNamespace() ? -1
            : ns > d->getNamespace() ? 1
            : url.compare(d->getUrl());

      if (c < 0)
        u = p;
      else if (c > 0)
        l = p;
      else
      {
        log_debug("article found after " << itcount << " iterations in file \"" << getFilename() << "\" at index " << p);
        return std::pair<bool, entry_index_t>(true, entry_index_t(p));
      }
    }

    auto d = getDirent(entry_index_t(l));
    int c = url.compare(d->getUrl());

    if (c == 0)
    {
      log_debug("article found after " << itcount << " iterations in file \"" << getFilename() << "\" at index " << l);
      return std::pair<bool, entry_index_t>(true, entry_index_t(l));
    }

    log_debug("article not found after " << itcount << " iterations (\"" << d.getUrl() << "\" does not match)");
    return std::pair<bool, entry_index_t>(false, entry_index_t(c < 0 ? l : u));
  }

  std::pair<bool, entry_index_t> FileImpl::findx(const std::string& url)
  {
    size_t start = 0;
    if (url[0] == '/') {
      start = 1;
    }
    if (url.size() < (2+start) || url[1+start] != '/')
      return std::pair<bool, entry_index_t>(false, entry_index_t(0));
    return findx(url[start], url.substr(2+start));
  }

  std::pair<bool, entry_index_t> FileImpl::findxByTitle(char ns, const std::string& title)
  {
    log_debug("find article by title " << ns << " \"" << title << "\", in file \"" << getFilename() << '"');

    entry_index_type l = entry_index_type(getNamespaceBeginOffset(ns));
    entry_index_type u = entry_index_type(getNamespaceEndOffset(ns));

    if (l == u)
    {
      log_debug("namespace " << ns << " not found");
      return std::pair<bool, entry_index_t>(false, entry_index_t(0));
    }

    unsigned itcount = 0;
    while (u - l > 1)
    {
      ++itcount;
      entry_index_type p = l + (u - l) / 2;
      auto d = getDirentByTitle(entry_index_t(p));

      int c = ns < d->getNamespace() ? -1
            : ns > d->getNamespace() ? 1
            : title.compare(d->getTitle());

      if (c < 0)
        u = p;
      else if (c > 0)
        l = p;
      else
      {
        log_debug("article found after " << itcount << " iterations in file \"" << getFilename() << "\" at index " << p);
        return std::pair<bool, entry_index_t>(true, entry_index_t(p));
      }
    }

    auto d = getDirentByTitle(entry_index_t(l));
    int c = title.compare(d->getTitle());

    if (c == 0)
    {
      log_debug("article found after " << itcount << " iterations in file \"" << getFilename() << "\" at index " << l);
      return std::pair<bool, entry_index_t>(true, entry_index_t(l));
    }

    log_debug("article not found after " << itcount << " iterations (\"" << d.getTitle() << "\" does not match)");
    return std::pair<bool, entry_index_t>(false, entry_index_t(c < 0 ? l : u));
  }


  std::pair<FileCompound::const_iterator, FileCompound::const_iterator>
  FileImpl::getFileParts(offset_t offset, zsize_t size)
  {
    return zimFile->locate(offset, size);
  }

  std::shared_ptr<const Dirent> FileImpl::getDirent(entry_index_t idx)
  {
    log_trace("FileImpl::getDirent(" << idx << ')');

    if (idx >= getCountArticles())
      throw std::out_of_range("entry index out of range");

    pthread_mutex_lock(&direntCacheLock);
    auto v = direntCache.getx(idx);
    if (v.first)
    {
      log_debug("dirent " << idx << " found in cache; hits "
                << direntCache.getHits() << " misses "
                << direntCache.getMisses() << " ratio "
                << direntCache.hitRatio() * 100 << "% fillfactor "
                << direntCache.fillfactor());
      pthread_mutex_unlock(&direntCacheLock);
      return v.second;
    }

    log_debug("dirent " << idx << " not found in cache; hits "
              << direntCache.getHits() << " misses " << direntCache.getMisses()
              << " ratio " << direntCache.hitRatio() * 100 << "% fillfactor "
              << direntCache.fillfactor());
    pthread_mutex_unlock(&direntCacheLock);

    offset_t indexOffset = getOffset(urlPtrOffsetReader.get(), idx.v);
    // We don't know the size of the dirent because it depends of the size of
    // the title, url and extra parameters.
    // This is a pitty but we have no choices.
    // We cannot take a buffer of the size of the file, it would be really inefficient.
    // Let's do try, catch and retry while chosing a smart value for the buffer size.
    // Most dirent will be "Article" entry (header's size == 16) without extra parameters.
    // Let's hope that url + title size will be < 256 and if not try again with a bigger size.

    pthread_mutex_lock(&bufferDirentLock);
    zsize_t bufferSize = zsize_t(256);
    // On very small file, the offset + 256 is higher than the size of the file,
    // even if the file is valid.
    // So read only to the end of the file.
    auto totalSize = zimReader->size();
    if (indexOffset.v + 256 > totalSize.v) bufferSize = zsize_t(totalSize.v-indexOffset.v);
    std::shared_ptr<const Dirent> dirent;
    while (true) {
        bufferDirentZone.reserve(size_type(bufferSize));
        zimReader->read(bufferDirentZone.data(), indexOffset, bufferSize);
        const MemoryBuffer<false> direntBuffer(bufferDirentZone.data(), bufferSize);
        try {
          dirent = std::make_shared<const Dirent>(direntBuffer);
        } catch (InvalidSize&) {
          // buffer size is not enougth, try again :
          bufferSize += 256;
          continue;
        }
        // Success !
        break;
    }
    pthread_mutex_unlock(&bufferDirentLock);

    log_debug("dirent read from " << indexOffset);
    pthread_mutex_lock(&direntCacheLock);
    direntCache.put(idx, dirent);
    pthread_mutex_unlock(&direntCacheLock);

    return dirent;
  }

  std::shared_ptr<const Dirent> FileImpl::getDirentByTitle(entry_index_t idx)
  {
    return getDirent(getIndexByTitle(idx));
  }

  entry_index_t FileImpl::getIndexByTitle(entry_index_t idx) const
  {
    if (idx >= getCountArticles())
      throw std::out_of_range("entry index out of range");

    entry_index_t ret(titleIndexReader->read_uint<entry_index_type>(
                            offset_t(sizeof(entry_index_t)*idx.v)));

    return ret;
  }

  entry_index_t FileImpl::getIndexByClusterOrder(entry_index_t idx) const
  {
      std::call_once(orderOnceFlag, [this]
      {
          auto nb_articles = this->getCountArticles().v;
          articleListByCluster.reserve(nb_articles);

          for(zim::entry_index_type i = 0; i < nb_articles; i++)
          {
              // This is the offset of the dirent in the zimFile
              auto indexOffset = getOffset(urlPtrOffsetReader.get(), i);
              // Get the mimeType of the dirent (offset 0) to know the type of the dirent
              uint16_t mimeType = zimReader->read_uint<uint16_t>(indexOffset);
              if (mimeType==Dirent::redirectMimeType || mimeType==Dirent::linktargetMimeType || mimeType == Dirent::deletedMimeType) {
                articleListByCluster.push_back(std::make_pair(0, i));
              } else {
                // If it is a classic article, get the clusterNumber (at offset 8)
                auto clusterNumber = zimReader->read_uint<zim::cluster_index_type>(indexOffset+offset_t(8));
                articleListByCluster.push_back(std::make_pair(clusterNumber, i));
              }
          }
          std::sort(articleListByCluster.begin(), articleListByCluster.end());
      });

      if (idx >= getCountArticles())
        throw std::out_of_range("entry index out of range");
      return entry_index_t(articleListByCluster[idx.v].second);
  }

  std::shared_ptr<const Cluster> FileImpl::getCluster(cluster_index_t idx)
  {
    if (idx >= getCountClusters())
      throw ZimFileFormatError("cluster index out of range");

    pthread_mutex_lock(&clusterCacheLock);
    auto cluster(clusterCache.get(idx));
    pthread_mutex_unlock(&clusterCacheLock);
    if (cluster)
    {
      log_debug("cluster " << idx << " found in cache; hits " << clusterCache.getHits() << " misses " << clusterCache.getMisses() << " ratio " << clusterCache.hitRatio() * 100 << "% fillfactor " << clusterCache.fillfactor());
      return cluster;
    }

    offset_t clusterOffset(getClusterOffset(idx));
    log_debug("read cluster " << idx << " from offset " << clusterOffset);
    CompressionType comp;
    bool extended;
    std::shared_ptr<const Reader> reader = zimReader->sub_clusterReader(clusterOffset, &comp, &extended);
    cluster = std::shared_ptr<Cluster>(new Cluster(reader, comp, extended));

    log_debug("put cluster " << idx << " into cluster cache; hits " << clusterCache.getHits() << " misses " << clusterCache.getMisses() << " ratio " << clusterCache.hitRatio() * 100 << "% fillfactor " << clusterCache.fillfactor());
    pthread_mutex_lock(&clusterCacheLock);
    clusterCache.put(idx, cluster);
    pthread_mutex_unlock(&clusterCacheLock);

    return cluster;
  }

  offset_t FileImpl::getOffset(const Reader* reader, size_t idx)
  {
    offset_t offset(reader->read_uint<offset_type>(offset_t(sizeof(offset_type)*idx)));
    return offset;
  }

  offset_t FileImpl::getClusterOffset(cluster_index_t idx)
  {
    return getOffset(clusterOffsetReader.get(), idx.v);
  }

  offset_t FileImpl::getBlobOffset(cluster_index_t clusterIdx, blob_index_t blobIdx)
  {
    auto cluster = getCluster(clusterIdx);
    if (cluster->isCompressed())
      return offset_t(0);
    return getClusterOffset(clusterIdx) + offset_t(1) + cluster->getBlobOffset(blobIdx);
  }

  entry_index_t FileImpl::getNamespaceBeginOffset(char ch)
  {
    log_trace("getNamespaceBeginOffset(" << ch << ')');

    pthread_mutex_lock(&namespaceBeginLock);
    NamespaceCache::const_iterator it = namespaceBeginCache.find(ch);
    if (it != namespaceBeginCache.end())
    {
      entry_index_t ret(it->second);
      pthread_mutex_unlock(&namespaceBeginLock);
      return ret;
    }
    pthread_mutex_unlock(&namespaceBeginLock);

    entry_index_type lower = 0;
    entry_index_type upper = entry_index_type(getCountArticles());
    auto d = getDirent(entry_index_t(0));
    while (upper - lower > 1)
    {
      entry_index_type m = lower + (upper - lower) / 2;
      auto d = getDirent(entry_index_t(m));
      if (d->getNamespace() >= ch)
        upper = m;
      else
        lower = m;
    }

    entry_index_t ret = entry_index_t(d->getNamespace() < ch ? upper : lower);
    pthread_mutex_lock(&namespaceBeginLock);
    namespaceBeginCache[ch] = ret;
    pthread_mutex_unlock(&namespaceBeginLock);

    return ret;
  }

  entry_index_t FileImpl::getNamespaceEndOffset(char ch)
  {
    log_trace("getNamespaceEndOffset(" << ch << ')');

    pthread_mutex_lock(&namespaceEndLock);
    NamespaceCache::const_iterator it = namespaceEndCache.find(ch);
    if (it != namespaceEndCache.end())
    {
      entry_index_t ret = it->second;
      pthread_mutex_unlock(&namespaceEndLock);
      return ret;
    }
    pthread_mutex_unlock(&namespaceEndLock);

    entry_index_type lower = 0;
    entry_index_type upper = entry_index_type(getCountArticles());
    log_debug("namespace " << ch << " lower=" << lower << " upper=" << upper);
    while (upper - lower > 1)
    {
      entry_index_type m = lower + (upper - lower) / 2;
      auto d = getDirent(entry_index_t(m));
      if (d->getNamespace() > ch)
        upper = m;
      else
        lower = m;
      log_debug("namespace " << d->getNamespace() << " m=" << m << " lower=" << lower << " upper=" << upper);
    }

    pthread_mutex_lock(&namespaceEndLock);
    namespaceEndCache[ch] = entry_index_t(upper);
    pthread_mutex_unlock(&namespaceEndLock);

    return entry_index_t(upper);
  }

  std::string FileImpl::getNamespaces()
  {
    std::string namespaces;

    auto d = getDirent(entry_index_t(0));
    namespaces = d->getNamespace();

    entry_index_t idx(0);
    while ((idx = getNamespaceEndOffset(d->getNamespace())) < getCountArticles())
    {
      d = getDirent(idx);
      namespaces += d->getNamespace();
    }

    return namespaces;
  }

  const std::string& FileImpl::getMimeType(uint16_t idx) const
  {
    if (idx > mimeTypes.size())
    {
      std::ostringstream msg;
      msg << "unknown mime type code " << idx;
      throw ZimFileFormatError(msg.str());
    }

    return mimeTypes[idx];
  }

  std::string FileImpl::getChecksum()
  {
    if (!header.hasChecksum())
      return std::string();

    std::shared_ptr<const Buffer> chksum;
    try {
      chksum = zimReader->get_buffer(offset_t(header.getChecksumPos()), zsize_t(16));
    } catch (...)
    {
      log_warn("error reading checksum");
      return std::string();
    }

    char hexdigest[33];
    hexdigest[32] = '\0';
    static const char hex[] = "0123456789abcdef";
    char* p = hexdigest;
    for (int i = 0; i < 16; ++i)
    {
      uint8_t v = chksum->at(offset_t(i));
      *p++ = hex[v >> 4];
      *p++ = hex[v & 0xf];
    }
    log_debug("chksum=" << hexdigest);
    return hexdigest;
  }

  bool FileImpl::verify()
  {
    if (!header.hasChecksum())
      return false;

    struct zim_MD5_CTX md5ctx;
    zim_MD5Init(&md5ctx);

    offset_type checksumPos = header.getChecksumPos();
    offset_type currentPos = 0;
    for(auto part = zimFile->begin();
        part != zimFile->end();
        part++) {
      std::ifstream stream(part->second->filename());
      char ch;
      for(/*NOTHING*/ ; currentPos < checksumPos && stream.get(ch).good(); currentPos++) {
        zim_MD5Update(&md5ctx, reinterpret_cast<const uint8_t*>(&ch), 1);
      }
      if (stream.bad()) {
        perror("error while reading file");
        return false;
      }
      if (currentPos == checksumPos) {
        break;
      }
    }

    if (currentPos != checksumPos) {
      return false;
    }

    unsigned char chksumCalc[16];
    auto chksumFile = zimReader->get_buffer(offset_t(header.getChecksumPos()), zsize_t(16));

    zim_MD5Final(chksumCalc, &md5ctx);
    if (std::memcmp(chksumFile->data(), chksumCalc, 16) != 0)
    {
      return false;
    }

    return true;
  }

  time_t FileImpl::getMTime() const {
    return zimFile->getMTime();
  }

  zim::zsize_t FileImpl::getFilesize() const {
    return zimFile->fsize();
  }

  bool FileImpl::is_multiPart() const {
    return zimFile->is_multiPart();
  }
}
