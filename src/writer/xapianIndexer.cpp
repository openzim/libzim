/*
 * Copyright (C) 2021 Maneesh P M <manu.pm55@gmail.com>
 * Copyright (C) 2018-2021 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2011 Emmanuel Engelhart <kelson@kiwix.org>
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

#include "xapianIndexer.h"
#include "libzim-resources.h"
#include "fs.h"
#include "tools.h"
#include "../constants.h"
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <cassert>

using namespace zim::writer;

/* Constructor */
XapianIndexer::XapianIndexer(const std::string& indexPath, const std::string& language, IndexingMode indexingMode, const bool verbose)
    : indexPath(indexPath),
      language(language),
      indexingMode(indexingMode)
{
  /* Build ICU Local object to retrieve ISO-639 language code (from
     ISO-639-3) */
  icu::Locale languageLocale(language.c_str());
  stemmer_language = languageLocale.getLanguage();

  /* Read the stopwords */
  std::string stopWord;
  try {
    this->stopwords = getResource("stopwords/" + language);
  } catch(ResourceNotFound& e) {}
  std::istringstream file(this->stopwords);
  while (std::getline(file, stopWord, '\n')) {
    this->stopper.add(stopWord);
  }
}

XapianIndexer::~XapianIndexer()
{
  if (!indexPath.empty()) {
    try {
#ifndef _WIN32
//[TODO] Implement remove for windows
      zim::DEFAULTFS::remove(indexPath + ".tmp");
      zim::DEFAULTFS::remove(indexPath);
#endif
    } catch (...) {
      /* Do not raise */
    }
  }
}

/*
 * `valuesmap` is a metadata associated with the Xapian database. We are using it
 * to attach slot numbers of each document in the index to the value they are storing.
 * These values and slot numbers are used in collapsing, filtering etc.
 *
 * Title index:
 * Slot 0: Title of the article. Used in collapsing articles with same name.
 * Slot 1: path/redirectPath of the article. Used in collapsing duplicates(redirects).
 *
 * Fulltext Index:
 * Slot 0: Title of the article. Used in collapsing articles with same name.
 * Slot 1: Word count of the article.
 * Slot 2: Geo position of the article. Used for geo-filtering.
 *
 * `kind` metadata indicate whether the database is a title or a fulltext index.
 *
 * `data` metadata indicate the type of data stored in the index. A value of "fullPath"
 * means the data stores the complete path with a namespace.
 */

void XapianIndexer::indexingPrelude()
{
  writableDatabase = Xapian::WritableDatabase(indexPath + ".tmp", Xapian::DB_CREATE_OR_OVERWRITE | Xapian::DB_NO_TERMLIST);

  switch (indexingMode) {
    case IndexingMode::TITLE:
      writableDatabase.set_metadata("valuesmap", "title:0;targetPath:1");
      writableDatabase.set_metadata("kind", "title");
      writableDatabase.set_metadata("data", "fullPath");
      break;
    case IndexingMode::FULL:
      writableDatabase.set_metadata("valuesmap", "title:0;wordcount:1;geo.position:2");
      writableDatabase.set_metadata("kind", "fulltext");
      writableDatabase.set_metadata("data", "fullPath");
      break;
  }
  writableDatabase.set_metadata("language", language);
  writableDatabase.set_metadata("stopwords", stopwords);
  writableDatabase.begin_transaction(true);
}

/*
 * For title index, index the full path with namespace as data of the document.
 * The targetPath in valuesmap will store the path without namespace.
 * TODO:
 * Currently for title index we are storing path twice (redirectPath/path in
 * valuesmap and path in index data). In the future, we want to keep only one of
 * these(index data if possible) to reduce index size while supporting the
 * collapse on path feature.
 */

void XapianIndexer::indexTitle(const std::string& path, const std::string& title, const std::string& targetPath)
{
  assert(indexingMode == IndexingMode::TITLE);
  Xapian::Stem stemmer;
  Xapian::TermGenerator indexer;
  try {
    stemmer = Xapian::Stem(stemmer_language);
    indexer.set_stemmer(stemmer);
    indexer.set_stemming_strategy(Xapian::TermGenerator::STEM_SOME);
  } catch (...) {}
  Xapian::Document currentDocument;
  currentDocument.clear_values();

  std::string fullPath = "C/" + path;
  currentDocument.set_data(fullPath);
  indexer.set_document(currentDocument);

  std::string unaccentedTitle = zim::removeAccents(title);

  currentDocument.add_value(0, title);
  if (targetPath.empty()) {
    currentDocument.add_value(1, path);
  } else {
    currentDocument.add_value(1, targetPath);
  }

  if (!unaccentedTitle.empty()) {
    std::string anchoredTitle = ANCHOR_TERM + unaccentedTitle;
    indexer.index_text(anchoredTitle, 1);
  }

  /* add to the database */
  writableDatabase.add_document(currentDocument);
  empty = false;
}

void XapianIndexer::flush()
{
  this->writableDatabase.commit_transaction();
  this->writableDatabase.begin_transaction(true);
}

void XapianIndexer::indexingPostlude()
{
  this->flush();
  this->writableDatabase.commit_transaction();
  this->writableDatabase.commit();
  this->writableDatabase.compact(indexPath, Xapian::DBCOMPACT_SINGLE_FILE);
  this->writableDatabase.close();
}

