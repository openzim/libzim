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

/* Count word */
unsigned int countWords(const string& text)
{
  unsigned int numWords = 1;
  unsigned int length = text.size();

  for (unsigned int i = 0; i < length;) {
    while (i < length && text[i] != ' ') {
      i++;
    }
    numWords++;
    i++;
  }

  return numWords;
}

/* Constructor */
XapianIndexer::XapianIndexer(const std::string& language, const bool verbose)
    : language(language)
{
  /* Build ICU Local object to retrieve ISO-639 language code (from
     ISO-639-3) */
  icu::Locale languageLocale(language.c_str());

  /* Configuring language base steemming */
  try {
    this->stemmer = Xapian::Stem(languageLocale.getLanguage());
    this->indexer.set_stemmer(this->stemmer);
    this->indexer.set_stemming_strategy(Xapian::TermGenerator::STEM_ALL);
  } catch (...) {
    std::cout << "No steemming for language '" << languageLocale.getLanguage()
              << "'" << std::endl;
  }

  /* Read the stopwords */
  std::string stopWord;
  try {
    this->stopwords = getResource("stopwords/" + language);
  } catch(ResourceNotFound& e) {}
  std::istringstream file(this->stopwords);
  while (std::getline(file, stopWord, '\n')) {
    this->stopper.add(stopWord);
  }

  this->indexer.set_stopper(&(this->stopper));
  this->indexer.set_stopper_strategy(Xapian::TermGenerator::STOP_ALL);
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

void XapianIndexer::indexingPrelude(const string indexPath_)
{
  indexPath = indexPath_;
  this->writableDatabase = Xapian::WritableDatabase(
      indexPath + ".tmp", Xapian::DB_CREATE_OR_OVERWRITE);
  this->writableDatabase.set_metadata("valuesmap", "title:0;wordcount:1;geo.position:2");
  this->writableDatabase.set_metadata("language", language);
  this->writableDatabase.set_metadata("stopwords", stopwords);
  this->writableDatabase.set_metadata("prefixes", "S");
  this->writableDatabase.begin_transaction(true);
}

void XapianIndexer::index(const zim::writer::Article* article)
{
  /* Put the data in the document */
  Xapian::Document currentDocument;
  currentDocument.clear_values();
  auto url = article->getUrl();
  currentDocument.set_data(url.getLongUrl());
  indexer.set_document(currentDocument);
  zim::MyHtmlParser htmlParser;

  try {
    htmlParser.parse_html(article->getData(), "UTF-8", true);
  } catch (...) {
  }

  if (htmlParser.dump.find("NOINDEX") != string::npos)
  {
    return;
  }

  std::string accentedTitle = (htmlParser.title.empty() ? article->getTitle() : htmlParser.title);
  std::string title = zim::removeAccents(accentedTitle);
  std::string keywords = zim::removeAccents(htmlParser.keywords);
  std::string content = zim::removeAccents(htmlParser.dump);

  currentDocument.add_value(0, title);

  std::stringstream countWordStringStream;
  countWordStringStream << countWords(htmlParser.dump);
  currentDocument.add_value(1, countWordStringStream.str());

  if (htmlParser.has_geoPosition) {
    auto geoPosition = Xapian::LatLongCoord(
        htmlParser.latitude, htmlParser.longitude).serialise();
    currentDocument.add_value(2, geoPosition);
  }

  /* Index the title */
  if (!title.empty()) {
    this->indexer.index_text_without_positions(
        title, this->getTitleBoostFactor(content.size()));
    this->indexer.index_text(title, 1, "S");
  }

  /* Index the keywords */
  if (!keywords.empty()) {
    this->indexer.index_text_without_positions(keywords, keywordsBoostFactor);
  }

  /* Index the content */
  if (!content.empty()) {
    this->indexer.index_text_without_positions(content);
  }

  /* add to the database */
  this->writableDatabase.add_document(currentDocument);
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

XapianMetaArticle* XapianIndexer::getMetaArticle()
{
  return new XapianMetaArticle(this);
}

zim::size_type XapianMetaArticle::getSize() const
{
  std::ifstream in(indexer->getIndexPath(), std::ios::binary|std::ios::ate);
  return in.tellg();
}

std::string XapianMetaArticle::getFilename() const
{
  return indexer->getIndexPath();
}

zim::Blob XapianMetaArticle::getData() const
{
  throw std::logic_error("We should not pass here.");
  return zim::Blob();
}
