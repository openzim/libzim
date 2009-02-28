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

#ifndef ZIM_FILE_H
#define ZIM_FILE_H

#include <string>
#include <iterator>
#include <zim/zim.h>
#include <cxxtools/smartptr.h>
#include <zim/fileimpl.h>

namespace zim
{
  class Article;
  class Dirent;

  class File
  {
      cxxtools::SmartPtr<FileImpl> impl;

    public:
      File()
        { }
      explicit File(const std::string& fname);
      explicit File(FileImpl* impl);

      std::string readData(offset_type off, size_type count);
      std::string uncompressData(const Dirent& dirent, const std::string& data);

      const std::string& getFilename() const   { return impl->getFilename(); }
      const Fileheader& getFileheader() const  { return impl->getFileheader(); }

      Article getArticle(char ns, const std::string& url, bool collate = false);
      Article getArticle(char ns, const QUnicodeString& url, bool collate = false);
      Article getArticle(size_type idx);
      Dirent getDirent(size_type idx);
      offset_type getDirentOffset(size_type idx) const  { return impl->getDirentOffset(idx); }
      size_type getCountArticles() const;

      size_type getNamespaceBeginOffset(char ch);
      size_type getNamespaceEndOffset(char ch);
      size_type getNamespaceCount(char ns)
        { return getNamespaceEndOffset(ns) - getNamespaceBeginOffset(ns); }

      std::string getNamespaces();
      bool hasNamespace(char ch);

      class const_iterator;

      const_iterator begin();
      const_iterator end();
      const_iterator find(char ns, const std::string& url, bool collate = false);
      const_iterator find(char ns, const QUnicodeString& url, bool collate = false);

      operator bool() const  { return impl.getPointer() != 0; }
  };

}

#endif // ZIM_FILE_H

