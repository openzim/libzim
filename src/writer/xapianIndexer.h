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

#include <zim/writer/item.h>

#include <unicode/locid.h>
#include <xapian.h>
#include <zim/blob.h>
#include "xapian/myhtmlparse.h"


namespace zim {
  namespace writer {
    class IndexTask;
  }
}
class XapianIndexer;

enum class IndexingMode {
  TITLE,
  FULL
};

class XapianMetaItem : public zim::writer::Item
{
 private:
  XapianIndexer* indexer;
  IndexingMode mode;
  mutable std::string data;

 public:
  XapianMetaItem(XapianIndexer* indexer, IndexingMode mode) : indexer(indexer), mode(mode)
  {}
  virtual ~XapianMetaItem() = default;
  virtual zim::Blob getData() const;
  virtual zim::writer::Url getUrl() const {
    switch (mode) {
      case IndexingMode::FULL:
        return zim::writer::Url('X', "fulltext/xapian");
      case IndexingMode::TITLE:
        return zim::writer::Url('X', "title/xapian");
    }
    return zim::writer::Url();
  }
  virtual std::string getTitle() const {
    switch (mode) {
      case IndexingMode::FULL:
        return "Xapian Fulltext Index";
      case IndexingMode::TITLE:
        return "Xapian Title Index";
    }
    return "";
  }
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
  XapianIndexer(const std::string& language, IndexingMode mode, bool verbose);
  virtual ~XapianIndexer();
  std::string getIndexPath() { return indexPath; }
  void indexingPrelude(const string indexPath);
  void index(const zim::writer::Item* item);
  void flush();
  void indexingPostlude();
  XapianMetaItem* getMetaItem();

 protected:
  void indexTitle(const zim::writer::Item* item);
  void indexFull(const zim::writer::Item* item);

  Xapian::WritableDatabase writableDatabase;
  std::string stemmer_language;
  Xapian::SimpleStopper stopper;
  std::string indexPath;
  std::string language;
  std::string stopwords;
  IndexingMode indexingMode;

 friend class zim::writer::IndexTask;
};

#endif  // LIBZIM_WRITER_XAPIANINDEXER_H
