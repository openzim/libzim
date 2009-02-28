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

#include <zim/fileimpl.h>
#include <zim/error.h>
#include <zim/article.h>
#include <zim/dirent.h>
#include <zim/qunicode.h>
#include <zim/fileheader.h>
#include <cxxtools/log.h>
#include <zim/inflatestream.h>
#include <zim/bunzip2stream.h>
#include <sstream>

log_define("zim.file.impl")

namespace zim
{
  //////////////////////////////////////////////////////////////////////
  // FileImpl
  //
  FileImpl::FileImpl(const char* fname)
    : zimFile(fname),
      uncompressCache(16)
  {
    if (!zimFile)
      throw ZenoFileFormatError(std::string("can't open zim-file \"") + fname + '"');

    filename = fname;

    zimFile >> header;
    if (zimFile.fail())
      throw ZenoFileFormatError("error reading zim-file header");

    size_type rCount = header.getCount();
    offset_type rIndexPos = header.getIndexPos();
    size_type rIndexLen = header.getIndexLen();
    offset_type rIndexPtrPos = header.getIndexPtrPos();
    size_type rIndexPtrLen = header.getIndexPtrLen();

    log_debug("magic=" << header.getMagicNumber() << " version=" << header.getVersion() << " count=" << rCount << " indexpos=" << rIndexPos << " indexlen=" << rIndexLen << " indexptrpos=" << rIndexPtrPos << " indexptrlen=" << rIndexPtrLen);

    log_debug("read " << rIndexPtrLen << " bytes");
    std::vector<size_type> buffer(rCount);
    zimFile.seekg(rIndexPtrPos);
    zimFile.read(reinterpret_cast<char*>(&buffer[0]), rIndexPtrLen);

    indexOffsets.reserve(rCount);
    for (std::vector<size_type>::const_iterator it = buffer.begin();
         it != buffer.end(); ++it)
      indexOffsets.push_back(static_cast<offset_type>(rIndexPos + fromLittleEndian<size_type>(&*it)));

    log_debug("read " << indexOffsets.size() << " index-entries ready");
  }

  Article FileImpl::getArticle(char ns, const QUnicodeString& url, bool collate)
  {
    log_debug("get article " << ns << " \"" << url << '"');
    std::pair<bool, size_type> s = findArticle(ns, url, collate);
    if (!s.first)
    {
      log_warn("article \"" << url << "\" not found");
      return Article();
    }

    Dirent d = readDirentNolock(indexOffsets[s.second]);

    log_info("article \"" << url << "\" size " << d.getSize() << " mime-type " << d.getMimeType());

    return Article(s.second, d, File(this));
  }

  Article FileImpl::getArticle(char ns, const std::string& url, bool collate)
  {
    return getArticle(ns, QUnicodeString(url), collate);
  }

  std::pair<bool, size_type> FileImpl::findArticle(char ns, const QUnicodeString& title, bool collate)
  {
    log_debug("find article " << ns << " \"" << title << "\", " << collate << " in file \"" << filename << '"');

    if (getNamespaces().find(ns) == std::string::npos)
    {
      log_debug("namespace " << ns << " not found");
      return std::pair<bool, size_type>(false, 0);
    }

    cxxtools::MutexLock lock(mutex);

    IndexOffsetsType::size_type l = 0;
    IndexOffsetsType::size_type u = getCountArticles();

    unsigned itcount = 0;
    while (u - l > 1)
    {
      ++itcount;
      IndexOffsetsType::size_type p = l + (u - l) / 2;
      Dirent d = readDirentNolock(indexOffsets[p]);

      int c = ns < d.getNamespace() ? -1
            : ns > d.getNamespace() ? 1
            : (collate ? title.compareCollate(QUnicodeString(d.getTitle()))
                       : title.compare(QUnicodeString(d.getTitle())));
      if (c < 0)
        u = p;
      else if (c > 0)
        l = p;
      else
      {
        log_debug("article found after " << itcount << " iterations in file \"" << filename << "\" at index " << p);
        return std::pair<bool, size_type>(true, p);
      }
    }

    Dirent d = readDirentNolock(indexOffsets[l]);
    int c = collate ? title.compareCollate(QUnicodeString(d.getTitle()))
                    : title.compare(QUnicodeString(d.getTitle()));
    if (c == 0)
    {
      log_debug("article found after " << itcount << " iterations in file \"" << filename << "\" at index " << l);
      return std::pair<bool, size_type>(true, l);
    }

    log_debug("article not found (\"" << d.getTitle() << "\" does not match)");
    return std::pair<bool, size_type>(false, u);
  }

  Article FileImpl::getArticle(size_type idx)
  {
    log_debug("getArticle(" << idx << ')');

    if (idx >= getCountArticles())
      throw ZenoFileFormatError("article index out of range");

    cxxtools::MutexLock lock(mutex);
    Dirent d = readDirentNolock(indexOffsets[idx]);
    return Article(idx, d, File(this));
  }

  Dirent FileImpl::getDirent(size_type idx)
  {
    if (idx >= getCountArticles())
      throw ZenoFileFormatError("article index out of range");

    cxxtools::MutexLock lock(mutex);
    return readDirentNolock(indexOffsets[idx]);
  }

  size_type FileImpl::getNamespaceBeginOffset(char ch)
  {
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
    return d.getNamespace() < ch ? upper : lower;
  }

  size_type FileImpl::getNamespaceEndOffset(char ch)
  {
    log_debug("getNamespaceEndOffset(" << ch << ')');

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

  std::string FileImpl::readData(offset_type off, size_type count)
  {
    cxxtools::MutexLock lock(mutex);
    return readDataNolock(off, count);
  }

  std::string FileImpl::readDataNolock(offset_type off, size_type count)
  {
    zimFile.seekg(off);
    return readDataNolock(count);
  }

  std::string FileImpl::readDataNolock(size_type count)
  {
    std::string data;
    char buffer[256];
    while (count > 0)
    {
      zimFile.read(buffer, std::min(static_cast<size_type>(sizeof(buffer)), count));
      if (!zimFile)
        throw ZenoFileFormatError("format-error: error reading data");
      data.append(buffer, zimFile.gcount());
      count -= zimFile.gcount();
    }
    return data;
  }

  Dirent FileImpl::readDirentNolock(offset_type off)
  {
    //log_debug("read directory entry at offset " << off);
    zimFile.seekg(off);
    return readDirentNolock();
  }

  Dirent FileImpl::readDirentNolock()
  {
    Dirent dirent;
    zimFile >> dirent;
    if (!zimFile)
      throw ZenoFileFormatError("error reading directory index in \"" + filename + '"');
    return dirent;
  }

  std::string FileImpl::uncompressData(const Dirent& dirent, const std::string& data)
  {
    if (dirent.getArticleSize() == 0)
      return std::string();

    if (!dirent.isCompressed())
      return data;

    std::string uncompressedData = uncompressCache.get(dirent.getOffset());

    if (uncompressedData.empty())
    {
      log_debug("cache miss for offset " << dirent.getOffset() << ", size " << dirent.getSize());
      if (dirent.isCompressionZip())
      {
        log_debug("uncompress data (zlib)");
        std::ostringstream u;
        zim::InflateStream is(u);
        is << data << std::flush;
        uncompressedData = u.str();
      }
      else if (dirent.isCompressionBzip2())
      {
        log_debug("uncompress data (bzip2)");
        std::ostringstream u;
        zim::Bunzip2Stream is(u);
        is << data << std::flush;
        uncompressedData = u.str();
      }
      else if (dirent.isCompressionLzma())
      {
        // TODO
        log_debug("uncompress data (lzma)");
      }

      uncompressCache.put(dirent.getOffset(), uncompressedData);
    }
    else
      log_debug("cache hit for offset " << dirent.getOffset() << ", size " << dirent.getSize());

    return uncompressedData;
  }

}
