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

#include "xapianIndexer.h"
#include "libzim-resources.h"
#include "fs.h"
#include "tools.h"
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

void XapianIndexer::indexingPrelude()
{
  writableDatabase = Xapian::WritableDatabase(indexPath + ".tmp", Xapian::DB_CREATE_OR_OVERWRITE);
  switch (indexingMode) {
    case IndexingMode::TITLE:
      writableDatabase.set_metadata("valuesmap", "title:0");
      writableDatabase.set_metadata("kind", "title");
      break;
    case IndexingMode::FULL:
      writableDatabase.set_metadata("valuesmap", "title:0;wordcount:1;geo.position:2");
      writableDatabase.set_metadata("kind", "fulltext");
      break;
  }
  writableDatabase.set_metadata("language", language);
  writableDatabase.set_metadata("stopwords", stopwords);
  writableDatabase.begin_transaction(true);
}

void XapianIndexer::indexTitle(const std::string& path, const std::string& title)
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
  currentDocument.set_data(path);
  indexer.set_document(currentDocument);

  std::string unaccentedTitle = zim::removeAccents(title);

  currentDocument.add_value(0, title);

  if (!unaccentedTitle.empty()) {
    indexer.index_text(unaccentedTitle, 1);
  }

  /* add to the database */
  writableDatabase.add_document(currentDocument);
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

