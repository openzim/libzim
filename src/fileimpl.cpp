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

#include <zim/fileimpl.h>
#include <zim/error.h>
#include <zim/dirent.h>
#include <zim/file_compound.h>
#include <zim/file_reader.h>
#include "endian_tools.h"
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
  FileImpl::FileImpl(const char* fname)
    : zimFile(new FileCompound(fname)),
      zimReader(new FileReader(zimFile)),
      bufferDirentZone(256),
      direntCache(envValue("ZIM_DIRENTCACHE", DIRENT_CACHE_SIZE)),
      clusterCache(envValue("ZIM_CLUSTERCACHE", CLUSTER_CACHE_SIZE)),
      cacheUncompressedCluster(envValue("ZIM_CACHEUNCOMPRESSEDCLUSTER", false))
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
        std::cerr << "last offset (" << lastOffset << ") larger than file size (" << zimFile->fsize() << ')' << std::endl;
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

  const Dirent FileImpl::getDirent(size_type idx)
  {
    log_trace("FileImpl::getDirent(" << idx << ')');

    if (idx >= getCountArticles())
      throw ZimFileFormatError("article index out of range");

    std::pair<bool, const Dirent> v = direntCache.getx(idx);
    if (v.first)
    {
      log_debug("dirent " << idx << " found in cache; hits " << direntCache.getHits() << " misses " << direntCache.getMisses() << " ratio " << direntCache.hitRatio() * 100 << "% fillfactor " << direntCache.fillfactor());
      return v.second;
    }

    log_debug("dirent " << idx << " not found in cache; hits " << direntCache.getHits() << " misses " << direntCache.getMisses() << " ratio " << direntCache.hitRatio() * 100 << "% fillfactor " << direntCache.fillfactor());

    offset_type indexOffset = getOffset(urlPtrOffsetBuffer.get(), idx);
    // We don't know the size of the dirent because it depends of the size of
    // the title, url and extra parameters.
    // This is a pitty but we have no choices.
    // We cannot take a buffer of the size of the file, it would be really inefficient.
    // Let's do try, catch and retry while chosing a smart value for the buffer size.
    // Most dirent will be "Article" entry (header's size == 16) without extra parameters.
    // Let's hope that url + title size will be < 256 and if not try again with a bigger size.

    offset_type bufferSize = 256;
    Dirent dirent;
    while (true) {
        bufferDirentZone.reserve(bufferSize);
        zimReader->read(bufferDirentZone.data(), indexOffset, bufferSize);
        auto direntBuffer = std::unique_ptr<Buffer>(new MemoryBuffer<false>(bufferDirentZone.data(), bufferSize));
        try {
          dirent = Dirent(std::move(direntBuffer));
        } catch (InvalidSize) {
          // buffer size is not enougth, try again :
          bufferSize += 256;
          continue;
        }
        // Success !
        break;
    }

    log_debug("dirent read from " << indexOffset);
    direntCache.put(idx, dirent);

    return dirent;
  }

  const Dirent FileImpl::getDirentByTitle(size_type idx)
  {
    if (idx >= getCountArticles())
      throw ZimFileFormatError("article index out of range");
    return getDirent(getIndexByTitle(idx));
  }

  size_type FileImpl::getIndexByTitle(size_type idx)
  {
    if (idx >= getCountArticles())
      throw ZimFileFormatError("article index out of range");

    size_type ret = fromLittleEndian(titleIndexBuffer->as<size_type>(sizeof(size_type)*idx));

    return ret;
  }

  std::shared_ptr<Cluster> FileImpl::getCluster(size_type idx)
  {
    if (idx >= getCountClusters())
      throw ZimFileFormatError("cluster index out of range");

    auto cluster(clusterCache.get(idx));
    if (cluster)
    {
      log_debug("cluster " << idx << " found in cache; hits " << clusterCache.getHits() << " misses " << clusterCache.getMisses() << " ratio " << clusterCache.hitRatio() * 100 << "% fillfactor " << clusterCache.fillfactor());
      return cluster;
    }

    offset_type clusterOffset = getClusterOffset(idx);
    auto next_idx = idx + 1;
    offset_type nextClusterOffset = (next_idx < getCountClusters()) ? getClusterOffset(next_idx) : header.getChecksumPos();
    offset_type clusterSize = nextClusterOffset - clusterOffset;
    log_debug("read cluster " << idx << " from offset " << clusterOffset);
    CompressionType comp;
    std::shared_ptr<const Reader> reader = zimReader->sub_clusterReader(clusterOffset, clusterSize, &comp);
    cluster = std::shared_ptr<Cluster>(new Cluster(reader, comp));

    log_debug("put cluster " << idx << " into cluster cache; hits " << clusterCache.getHits() << " misses " << clusterCache.getMisses() << " ratio " << clusterCache.hitRatio() * 100 << "% fillfactor " << clusterCache.fillfactor());
    clusterCache.put(idx, cluster);

    return cluster;
  }

  offset_type FileImpl::getOffset(const Buffer* buffer, size_type idx)
  {
    offset_type offset = fromLittleEndian(buffer->as<offset_type>(sizeof(offset_type)*idx));
    return offset;
  }

  size_type FileImpl::getNamespaceBeginOffset(char ch)
  {
    log_trace("getNamespaceBeginOffset(" << ch << ')');

    NamespaceCache::const_iterator it = namespaceBeginCache.find(ch);
    if (it != namespaceBeginCache.end())
      return it->second;

    size_type lower = 0;
    size_type upper = getCountArticles();
    Dirent d = getDirent(0);
    while (upper - lower > 1)
    {
      size_type m = lower + (upper - lower) / 2;
      Dirent d = getDirent(m);
      if (d.getNamespace() >= ch)
        upper = m;
      else
        lower = m;
    }

    size_type ret = d.getNamespace() < ch ? upper : lower;
    namespaceBeginCache[ch] = ret;

    return ret;
  }

  size_type FileImpl::getNamespaceEndOffset(char ch)
  {
    log_trace("getNamespaceEndOffset(" << ch << ')');

    NamespaceCache::const_iterator it = namespaceEndCache.find(ch);
    if (it != namespaceEndCache.end())
      return it->second;

    size_type lower = 0;
    size_type upper = getCountArticles();
    log_debug("namespace " << ch << " lower=" << lower << " upper=" << upper);
    while (upper - lower > 1)
    {
      size_type m = lower + (upper - lower) / 2;
      Dirent d = getDirent(m);
      if (d.getNamespace() > ch)
        upper = m;
      else
        lower = m;
      log_debug("namespace " << d.getNamespace() << " m=" << m << " lower=" << lower << " upper=" << upper);
    }

    namespaceEndCache[ch] = upper;

    return upper;

  }

  std::string FileImpl::getNamespaces()
  {
    if (namespaces.empty())
    {
      Dirent d = getDirent(0);
      namespaces = d.getNamespace();

      size_type idx;
      while ((idx = getNamespaceEndOffset(d.getNamespace())) < getCountArticles())
      {
        d = getDirent(idx);
        namespaces += d.getNamespace();
      }

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
