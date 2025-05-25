/*
 * Copyright (C) 2021 Maneesh P M <manu.pm55@gmail.com>
 * Copyright (C) 2020-2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#include "xapianWorker.h"
#include "creatordata.h"

#include "xapianIndexer.h"

#include <stdexcept>
#include <mutex>
#include <zim/tools.h>

static std::mutex s_dbaccessLock;

namespace zim
{
  namespace writer
  {

    const unsigned int keywordsBoostFactor = 3;
    inline unsigned int getTitleBoostFactor(const unsigned int contentLength)
    {
      return contentLength / 500 + 1;
    }

    void IndexTask::run(CreatorData* data) {
      if (!mp_indexData->hasIndexData()) {
        return;
      }
      Xapian::Stem stemmer;
      Xapian::TermGenerator indexer;
      indexer.set_flags(Xapian::TermGenerator::FLAG_CJK_NGRAM);
      try {
        stemmer = Xapian::Stem(mp_indexer->stemmer_language);
        indexer.set_stemmer(stemmer);
        indexer.set_stemming_strategy(Xapian::TermGenerator::STEM_ALL);
      } catch (...) {
        // No stemming for language.
      }
      indexer.set_stopper(&mp_indexer->stopper);
      indexer.set_stopper_strategy(Xapian::TermGenerator::STOP_ALL);

      Xapian::Document document;
      indexer.set_document(document);

      std::string fullPath = "C/" + m_path;
      document.set_data(fullPath);
      document.add_value(0, mp_indexData->getTitle());
      document.add_value(1, Formatter() << mp_indexData->getWordCount());

      auto geoInfo = mp_indexData->getGeoPosition();
      if (std::get<0>(geoInfo)) {
        auto geoPosition = Xapian::LatLongCoord(
        std::get<1>(geoInfo), std::get<2>(geoInfo)).serialise();
        document.add_value(2, geoPosition);
      }

      /* Index the content */
      auto indexContent = mp_indexData->getContent();
      if (!indexContent.empty()) {
        indexer.index_text_without_positions(indexContent);
      }

      /* Index the title */
      auto indexTitle = mp_indexData->getTitle();
      if (!indexTitle.empty()) {
        indexer.index_text_without_positions(
          indexTitle, getTitleBoostFactor(indexContent.size()));
      }

      /* Index the keywords */
      auto indexKeywords = mp_indexData->getKeywords();
      if (!indexKeywords.empty()) {
        indexer.index_text_without_positions(indexKeywords, keywordsBoostFactor);
      }

      std::lock_guard<std::mutex> l(s_dbaccessLock);
      mp_indexer->writableDatabase.add_document(document);
      mp_indexer->empty = false;
    }
  }
}
