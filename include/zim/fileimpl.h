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

#ifndef ZIM_FILEIMPL_H
#define ZIM_FILEIMPL_H

#include <cxxtools/mutex.h>
#include <fstream>
#include <string>
#include <vector>
#include <cxxtools/refcounted.h>
#include <zim/zim.h>
#include <zim/qunicode.h>
#include <zim/fileheader.h>
#include <zim/cache.h>

namespace zim
{
  class Dirent;
  class Article;

  class FileImpl : public cxxtools::RefCounted
  {
      std::ifstream zimFile;
      Fileheader header;
      std::string filename;
      std::string namespaces;

      cxxtools::Mutex mutex;

      typedef std::vector<offset_type> IndexOffsetsType;
      IndexOffsetsType indexOffsets;

      std::string readDataNolock(size_type count);
      std::string readDataNolock(offset_type off, size_type count);
      Dirent readDirentNolock();
      Dirent readDirentNolock(offset_type off);

      Cache<offset_type, std::string> uncompressCache;

    public:
      FileImpl(const char* fname);

      const std::string& getFilename() const   { return filename; }
      const Fileheader& getFileheader() const  { return header; }

      Article getArticle(char ns, const QUnicodeString& url, bool collate);
      Article getArticle(char ns, const std::string& url, bool collate);
      Article getArticle(size_type idx);
      std::pair<bool, size_type> findArticle(char ns, const QUnicodeString& url, bool collate);
      Dirent getDirent(size_type idx);
      size_type getCountArticles() const  { return indexOffsets.size(); }
      offset_type getDirentOffset(size_type idx) const  { return indexOffsets[idx]; }

      size_type getNamespaceBeginOffset(char ch);
      size_type getNamespaceEndOffset(char ch);
      std::string getNamespaces();

      std::string readData(offset_type off, size_type count);
      std::string uncompressData(const Dirent& dirent, const std::string& data);
  };

}

#endif // ZIM_FILEIMPL_H

