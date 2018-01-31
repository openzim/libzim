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
#include "dirent.h"
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
#include "md5stream.h"

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
      titleIndexBuffer(0),
      urlPtrOffsetBuffer(0),
      clusterOffsetBuffer(0),
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
    try {
      header.read(zimReader->get_buffer(0, Fileheader::size));
    } catch (...) {
      throw ZimFileFormatError("error reading zim-file header");
    }

    // ptrOffsetBuffer
    offset_type size = header.getArticleCount() * 8;;
    urlPtrOffsetBuffer = zimReader->get_buffer(header.getUrlPtrPos(), size);

    // Create titleIndexBuffer
    size = header.getArticleCount() * 4;
    titleIndexBuffer = zimReader->get_buffer(header.getTitleIdxPos(), size);

    // clusterOffsetBuffer
    size = header.getClusterCount() * 8;
    clusterOffsetBuffer = zimReader->get_buffer(header.getClusterPtrPos(), size);


    if (getCountClusters() == 0)
      log_warn("no clusters found");
    else
    {
      offset_type lastOffset = getClusterOffset(getCountClusters() - 1);
      log_debug("last offset=" << lastOffset << " file size=" << zimFile->fsize());
      if (lastOffset > zimFile->fsize())
      {
        log_fatal("last offset (" << lastOffset << ") larger than file size (" << zimFile->fsize() << ')');
        throw ZimFileFormatError("last cluster offset larger than file size; file corrupt");
      }
    }

    // read mime types
    size = header.getUrlPtrPos() - header.getMimeListPos();
    auto buffer = zimReader->get_buffer(header.getMimeListPos(), size);
    offset_type current = 0;
    while (current < size)
    {
      offset_type len = strlen(buffer->data(current));

      if (len == 0) {
        break;
      }

      if (current + len >= size) {
       throw(ZimFileFormatError("Error getting mimelists."));
      }
      
      std::string mimeType(buffer->data(current), len);
      mimeTypes.push_back(mimeType);

      current += (len + 1);
    }
  }

  std::pair<bool, size_type> FileImpl::findx(char ns, const std::string& url)
  {
    log_debug("find article by url " << ns << " \"" << url << "\",  in file \"" << getFilename() << '"');
  
    size_type l = getNamespaceBeginOffset(ns);
    size_type u = getNamespaceEndOffset(ns);
  
    if (l == u)
    {
      log_debug("namespace " << ns << " not found");
      return std::pair<bool, size_type>(false, 0);
    }
  
    unsigned itcount = 0;
    while (u - l > 1)
    {
      ++itcount;
      size_type p = l + (u - l) / 2;
      auto d = getDirent(p);
  
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
        return std::pair<bool, size_type>(true, p);
      }
    }

    auto d = getDirent(l);
    int c = url.compare(d->getUrl());
  
    if (c == 0)
    {
      log_debug("article found after " << itcount << " iterations in file \"" << getFilename() << "\" at index " << l);
      return std::pair<bool, size_type>(true, l);
    }
  
    log_debug("article not found after " << itcount << " iterations (\"" << d.getUrl() << "\" does not match)");
    return std::pair<bool, size_type>(false, (c < 0 ? l : u));
  }
  
  std::pair<bool, size_type> FileImpl::findx(const std::string& url)
  {
    if (url.size() < 2 || url[1] != '/')
      return std::pair<bool, size_type>(false, 0);
    return findx(url[0], url.substr(2));
  }

  std::pair<bool, size_type> FileImpl::findxByTitle(char ns, const std::string& title)
  {
    log_debug("find article by title " << ns << " \"" << title << "\", in file \"" << getFilename() << '"');
  
    size_type l = getNamespaceBeginOffset(ns);
    size_type u = getNamespaceEndOffset(ns);
  
    if (l == u)
    {
      log_debug("namespace " << ns << " not found");
      return std::pair<bool, size_type>(false, 0);
    }
  
    unsigned itcount = 0;
    while (u - l > 1)
    {
      ++itcount;
      size_type p = l + (u - l) / 2;
      auto d = getDirentByTitle(p);
  
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
        return std::pair<bool, size_type>(true, p);
      }
    }
  
    auto d = getDirentByTitle(l);
    int c = title.compare(d->getTitle());

    if (c == 0)
    {
      log_debug("article found after " << itcount << " iterations in file \"" << getFilename() << "\" at index " << l);
      return std::pair<bool, size_type>(true, l);
    }
  
    log_debug("article not found after " << itcount << " iterations (\"" << d.getTitle() << "\" does not match)");
    return std::pair<bool, size_type>(false, (c < 0 ? l : u));
  }

  std::pair<FileCompound::const_iterator, FileCompound::const_iterator>
  FileImpl::getFileParts(offset_type offset, offset_type size)
  {
    return zimFile->locate(offset, size);
  }

  std::shared_ptr<const Dirent> FileImpl::getDirent(size_type idx)
  {
    log_trace("FileImpl::getDirent(" << idx << ')');

    if (idx >= getCountArticles())
      throw ZimFileFormatError("article index out of range");

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

    offset_type indexOffset = getOffset(urlPtrOffsetBuffer.get(), idx);
    // We don't know the size of the dirent because it depends of the size of
    // the title, url and extra parameters.
    // This is a pitty but we have no choices.
    // We cannot take a buffer of the size of the file, it would be really inefficient.
    // Let's do try, catch and retry while chosing a smart value for the buffer size.
    // Most dirent will be "Article" entry (header's size == 16) without extra parameters.
    // Let's hope that url + title size will be < 256 and if not try again with a bigger size.

    pthread_mutex_lock(&bufferDirentLock);
    offset_type bufferSize = 256;
    std::shared_ptr<const Dirent> dirent;
    while (true) {
        bufferDirentZone.reserve(bufferSize);
        zimReader->read(bufferDirentZone.data(), indexOffset, bufferSize);
        auto direntBuffer = std::unique_ptr<Buffer>(new MemoryBuffer<false>(bufferDirentZone.data(), bufferSize));
        try {
          dirent = std::make_shared<const Dirent>(std::move(direntBuffer));
        } catch (InvalidSize) {
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

  std::shared_ptr<const Dirent> FileImpl::getDirentByTitle(size_type idx)
  {
    if (idx >= getCountArticles())
      throw ZimFileFormatError("article index out of range");
    return getDirent(getIndexByTitle(idx));
  }

  size_type FileImpl::getIndexByTitle(size_type idx)
  {
    if (idx >= getCountArticles())
      throw ZimFileFormatError("article index out of range");

    size_type ret = titleIndexBuffer->as<size_type>(sizeof(size_type)*idx);

    return ret;
  }

  std::shared_ptr<const Cluster> FileImpl::getCluster(size_type idx)
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

    offset_type clusterOffset = getClusterOffset(idx);
    auto next_idx = idx + 1;
    offset_type nextClusterOffset = (next_idx < getCountClusters())
                                        ? getClusterOffset(next_idx)
                                        : (header.hasChecksum())
                                            ? header.getChecksumPos()
                                            : zimFile->fsize();
    offset_type clusterSize = nextClusterOffset - clusterOffset;
    log_debug("read cluster " << idx << " from offset " << clusterOffset);
    CompressionType comp;
    std::shared_ptr<const Reader> reader = zimReader->sub_clusterReader(clusterOffset, clusterSize, &comp);
    cluster = std::shared_ptr<Cluster>(new Cluster(reader, comp));

    log_debug("put cluster " << idx << " into cluster cache; hits " << clusterCache.getHits() << " misses " << clusterCache.getMisses() << " ratio " << clusterCache.hitRatio() * 100 << "% fillfactor " << clusterCache.fillfactor());
    pthread_mutex_lock(&clusterCacheLock);
    clusterCache.put(idx, cluster);
    pthread_mutex_unlock(&clusterCacheLock);

    return cluster;
  }

  offset_type FileImpl::getOffset(const Buffer* buffer, size_type idx)
  {
    offset_type offset = buffer->as<offset_type>(sizeof(offset_type)*idx);
    return offset;
  }

  offset_type FileImpl::getBlobOffset(size_type clusterIdx, size_type blobIdx)
  {
    auto cluster = getCluster(clusterIdx);
    if (cluster->isCompressed())
      return 0;
    return getClusterOffset(clusterIdx) + 1 + cluster->getBlobOffset(blobIdx);
  }


  size_type FileImpl::getNamespaceBeginOffset(char ch)
  {
    log_trace("getNamespaceBeginOffset(" << ch << ')');

    pthread_mutex_lock(&namespaceBeginLock);
    NamespaceCache::const_iterator it = namespaceBeginCache.find(ch);
    if (it != namespaceBeginCache.end())
    {
      size_type ret = it->second;
      pthread_mutex_unlock(&namespaceBeginLock);
      return ret;
    }
    pthread_mutex_unlock(&namespaceBeginLock);

    size_type lower = 0;
    size_type upper = getCountArticles();
    auto d = getDirent(0);
    while (upper - lower > 1)
    {
      size_type m = lower + (upper - lower) / 2;
      auto d = getDirent(m);
      if (d->getNamespace() >= ch)
        upper = m;
      else
        lower = m;
    }

    size_type ret = d->getNamespace() < ch ? upper : lower;
    pthread_mutex_lock(&namespaceBeginLock);
    namespaceBeginCache[ch] = ret;
    pthread_mutex_unlock(&namespaceBeginLock);

    return ret;
  }

  size_type FileImpl::getNamespaceEndOffset(char ch)
  {
    log_trace("getNamespaceEndOffset(" << ch << ')');

    pthread_mutex_lock(&namespaceEndLock);
    NamespaceCache::const_iterator it = namespaceEndCache.find(ch);
    if (it != namespaceEndCache.end())
    {
      size_type ret = it->second;
      pthread_mutex_unlock(&namespaceEndLock);
      return ret;
    }
    pthread_mutex_unlock(&namespaceEndLock);

    size_type lower = 0;
    size_type upper = getCountArticles();
    log_debug("namespace " << ch << " lower=" << lower << " upper=" << upper);
    while (upper - lower > 1)
    {
      size_type m = lower + (upper - lower) / 2;
      auto d = getDirent(m);
      if (d->getNamespace() > ch)
        upper = m;
      else
        lower = m;
      log_debug("namespace " << d->getNamespace() << " m=" << m << " lower=" << lower << " upper=" << upper);
    }

    pthread_mutex_lock(&namespaceEndLock);
    namespaceEndCache[ch] = upper;
    pthread_mutex_unlock(&namespaceEndLock);

    return upper;

  }

  std::string FileImpl::getNamespaces()
  {
    std::string namespaces;

    auto d = getDirent(0);
    namespaces = d->getNamespace();

    size_type idx;
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
      throw std::runtime_error(msg.str());
    }

    return mimeTypes[idx];
  }

  std::string FileImpl::getChecksum()
  {
    if (!header.hasChecksum())
      return std::string();

    std::shared_ptr<const Buffer> chksum;
    try {
      chksum = zimReader->get_buffer(header.getChecksumPos(), 16);
    } catch (BufferError)
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
      *p++ = hex[chksum->at(i) >> 4];
      *p++ = hex[chksum->at(i) & 0xf];
    }
    log_debug("chksum=" << hexdigest);
    return hexdigest;
  }

  bool FileImpl::verify()
  {
    if (!header.hasChecksum())
      return false;

    Md5stream md5;

    offset_type checksumPos = header.getChecksumPos();
    offset_type currentPos = 0;
    for(auto part = zimFile->begin();
        part != zimFile->end();
        part++) {
      std::fstream stream(part->second->filename());
      char ch;
      for(/*NOTHING*/ ; currentPos < checksumPos && stream.get(ch); currentPos++) {
        md5 << ch;
      }
      if (currentPos == checksumPos) {
        break;
      }
    }
           

    unsigned char chksumCalc[16];
    auto chksumFile = zimReader->get_buffer(header.getChecksumPos(), 16);

    md5.getDigest(chksumCalc);
    if (std::memcmp(chksumFile->data(), chksumCalc, 16) != 0)
      throw ZimFileFormatError("invalid checksum in zim file");

    return true;
  }

  time_t FileImpl::getMTime() const {
    return zimFile->getMTime();
  }

  zim::offset_type FileImpl::getFilesize() const {
    return zimFile->fsize();
  }

  bool FileImpl::is_multiPart() const {
    return zimFile->is_multiPart();
  }
}
