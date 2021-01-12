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


namespace zim {
namespace writer {

  class IndexTask;

enum class IndexingMode {
  TITLE,
  FULL
};

class XapianIndexer
{
 public:
  XapianIndexer(const std::string& indexPath, const std::string& language, IndexingMode mode, bool verbose);
  virtual ~XapianIndexer();
  std::string getIndexPath() { return indexPath; }
  void indexingPrelude();
  void flush();
  void indexingPostlude();

  void indexTitle(const std::string& path, const std::string& title);

 protected:
  Xapian::WritableDatabase writableDatabase;
  std::string stemmer_language;
  Xapian::SimpleStopper stopper;
  std::string indexPath;
  std::string language;
  std::string stopwords;
  IndexingMode indexingMode;

 friend class zim::writer::IndexTask;
};

}
}

#endif  // LIBZIM_WRITER_XAPIANINDEXER_H
