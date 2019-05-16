/*
 * Copyright 2011 Emmanuel Engelhart <kelson@kiwix.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU  General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef LIBZIM_WRITER_XAPIANINDEXER_H
#define LIBZIM_WRITER_XAPIANINDEXER_H

#include <zim/article.h>
#include <zim/writer/article.h>

#include <unicode/locid.h>
#include <xapian.h>
#include <zim/blob.h>
#include "xapian/myhtmlparse.h"

class XapianIndexer;

class XapianMetaArticle : public zim::writer::Article
{
 private:
  XapianIndexer* indexer;
  mutable std::string data;

 public:
  XapianMetaArticle(XapianIndexer* indexer) : indexer(indexer)
  {}
  virtual ~XapianMetaArticle() = default;
  virtual zim::Blob getData() const;
  virtual zim::writer::Url getUrl() const { return zim::writer::Url('X', "fulltext/xapian"); }
  virtual std::string getTitle() const { return "Xapian Fulltext Index"; }
  virtual std::string getMimeType() const { return "application/octet-stream+xapian"; }
  virtual bool isRedirect() const { return false; }
  virtual bool shouldIndex() const { return false; }
  virtual bool shouldCompress() const { return false; }
  virtual zim::writer::Url getRedirectUrl() const { return zim::writer::Url(); }
  virtual zim::size_type getSize() const;
  virtual std::string getFilename() const;
};

class XapianIndexer
{
 public:
  XapianIndexer(const std::string& language, bool verbose);
  virtual ~XapianIndexer();
  std::string getIndexPath() { return indexPath; }
  void indexingPrelude(const string indexPath);
  void index(const zim::writer::Article* article);
  void flush();
  void indexingPostlude();
  XapianMetaArticle* getMetaArticle();

 protected:
  unsigned int keywordsBoostFactor;
  inline unsigned int getTitleBoostFactor(const unsigned int contentLength)
  {
    return contentLength / 500 + 1;
  }

  Xapian::WritableDatabase writableDatabase;
  Xapian::Stem stemmer;
  Xapian::SimpleStopper stopper;
  Xapian::TermGenerator indexer;
  std::string indexPath;
  std::string language;
  std::string stopwords;
};

#endif  // LIBZIM_WRITER_XAPIANINDEXER_H
