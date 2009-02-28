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

#ifndef ZIM_ARTICLE_H
#define ZIM_ARTICLE_H

#include <string>
#include <zim/zim.h>
#include <zim/dirent.h>
#include <zim/qunicode.h>
#include <zim/file.h>

namespace zim
{
  class Article
  {
    public:
      typedef Dirent::CompressionType CompressionType;
      typedef Dirent::MimeType MimeType;

    private:
      Dirent dirent;
      std::string data;
      mutable std::string uncompressedData;
      File file;
      size_type idx;
      bool dataRead;

      void uncompressData() const;

    public:
      Article() : dataRead(true) { }
      Article(size_type idx_, const Dirent& dirent_, const File& file_,
              const std::string& data_, bool dummy)
        : dirent(dirent_),
          data(data_),
          file(file_),
          idx(idx_),
          dataRead(true)
          { }
      Article(size_type idx_, const Dirent& dirent_, const File& file_)
        : dirent(dirent_),
          file(file_),
          idx(idx_),
          dataRead(false)
          { }

      Dirent&       getDirent()                 { return dirent; }
      const Dirent& getDirent() const           { return dirent; }
      void          setDirent(const Dirent& d)  { dirent = d; }

      const std::string&
                  getParameter() const        { return dirent.getParameter(); }
      void        setParameter(const std::string& p)  { dirent.setParameter(p); }

      offset_type getDataOffset() const       { return dirent.getOffset(); }
      void        setDataOffset(offset_type o) { dirent.setOffset(o); }

      size_type   getDataLen() const          { return dirent.getSize(); }

      CompressionType getCompression() const  { return dirent.getCompression(); }
      bool        isCompressionZip() const    { return dirent.isCompressionZip(); }
      bool        isCompressionBzip2() const  { return dirent.isCompressionBzip2(); }
      bool        isCompressionLzma() const   { return dirent.isCompressionLzma(); }
      bool        isCompressed() const        { return isCompressionZip() || isCompressionLzma(); }

      QUnicodeString getTitle() const         { return QUnicodeString(dirent.getTitle()); }
      void        setTitle(const QUnicodeString& title)   { dirent.setTitle(title.getValue()); }

      MimeType    getLibraryMimeType() const  { return dirent.getMimeType(); }
      void        setLibraryMimeType(MimeType m) { dirent.setMimeType(m); }
      const std::string&
                  getMimeType() const;

      bool        getRedirectFlag() const         { return dirent.getRedirectFlag(); }
      void        setRedirectFlag(bool sw = true) { dirent.setRedirectFlag(sw); }

      char        getNamespace() const        { return dirent.getNamespace(); }
      void        setNamespace(char ns)       { dirent.setNamespace(ns); }

      size_type   getRedirectIndex() const      { return dirent.getRedirectIndex(); }
      void        setRedirectIndex(size_type o) { dirent.setRedirectIndex(o); }

      Article     getRedirectArticle() const;

      size_type   getArticleOffset() const      { return dirent.getArticleOffset(); }
      void        setArticleOffset(size_type o) { dirent.setArticleOffset(o); }

      size_type   getArticleSize() const        { return dirent.getArticleSize(); }
      void        setArticleSize(size_type s)   { dirent.setArticleSize(s); }

      operator bool()   { return getDataOffset() != 0; }

      bool operator< (const Article& a) const
        { return getNamespace() < a.getNamespace()
              || getNamespace() == a.getNamespace()
               && getTitle() < a.getTitle(); }

      const std::string& getRawData() const;

      std::string getData() const;
      void setData(const std::string& data_)   { data = data_; dataRead = true; }

      size_type getUncompressedLen() const
      {
        uncompressData();
        return uncompressedData.size();
      }

      const File& getFile() const             { return file; }
      File&       getFile()                   { return file; }
      void        setFile(const File& f)      { file = f; }

      size_type   getIndex() const            { return idx; }
      void        setIndex(size_type i)       { idx = i; }

      QUnicodeString getUrl() const           { return QUnicodeString(std::string(1, getNamespace()) + '/' + getDirent().getTitle()); }

      offset_type getIndexOffset() const      { return getFile().getFileheader().getIndexPos() + idx * 4; }
      offset_type getDirentOffset() const     { return getFile().getDirentOffset(idx); }
  };

}

#endif // ZIM_ARTICLE_H

