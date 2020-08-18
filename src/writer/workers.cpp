/*
 * Copyright (C) 2009 Tommi Maekitalo
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

#include "config.h"

#include "creatordata.h"
#include "cluster.h"
#include "debug.h"
#include <zim/blob.h>
#include "../endian_tools.h"
#include <algorithm>
#include <fstream>

#if defined(ENABLE_XAPIAN)
  #include "xapianIndexer.h"
#endif

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <limits>
#include <stdexcept>
#include <sstream>
#include "log.h"
#include "../fs.h"
#include "../tools.h"

static pthread_mutex_t s_dbaccessLock = PTHREAD_MUTEX_INITIALIZER;
std::atomic<unsigned long> zim::writer::ClusterTask::waiting_task(0);
std::atomic<unsigned long> zim::writer::IndexTask::waiting_task(0);

namespace zim
{
  namespace writer
  {

    const unsigned int keywordsBoostFactor = 3;
    inline unsigned int getTitleBoostFactor(const unsigned int contentLength)
    {
      return contentLength / 500 + 1;
    }


    void ClusterTask::run(CreatorData* data) {
      cluster->close();
    };

    void IndexTask::run(CreatorData* data) {
      Xapian::Stem stemmer;
      Xapian::TermGenerator indexer;
      try {
        stemmer = Xapian::Stem(data->indexer->stemmer_language);
        indexer.set_stemmer(stemmer);
        indexer.set_stemming_strategy(Xapian::TermGenerator::STEM_ALL);
      } catch (...) {
        // No stemming for language.
      }
      indexer.set_stopper(&data->indexer->stopper);
      indexer.set_stopper_strategy(Xapian::TermGenerator::STOP_ALL);

      auto indexData = p_item->getIndexData();

      if (!indexData->hasIndexData()) {
        return;
      }

      Xapian::Document document;
      indexer.set_document(document);

      document.set_data(p_item->getPath());
      document.add_value(0, p_item->getTitle());

      std::stringstream countWordStringStream;
      countWordStringStream << indexData->getWordCount();
      document.add_value(1, countWordStringStream.str());

      auto geoInfo = indexData->getGeoPosition();
      if (std::get<0>(geoInfo)) {
        auto geoPosition = Xapian::LatLongCoord(
        std::get<1>(geoInfo), std::get<2>(geoInfo)).serialise();
        document.add_value(2, geoPosition);
      }

      /* Index the content */
      auto indexContent = indexData->getContent();
      if (!indexContent.empty()) {
        indexer.index_text_without_positions(indexContent);
      }

      /* Index the title */
      auto indexTitle = indexData->getTitle();
      if (!indexTitle.empty()) {
        indexer.index_text_without_positions(
          indexTitle, getTitleBoostFactor(indexContent.size()));
      }

      /* Index the keywords */
      auto indexKeywords = indexData->getKeywords();
      if (!indexKeywords.empty()) {
        indexer.index_text_without_positions(indexKeywords, keywordsBoostFactor);
      }

      pthread_mutex_lock(&s_dbaccessLock);
      data->indexer->writableDatabase.add_document(document);
      pthread_mutex_unlock(&s_dbaccessLock);
    }

    void* taskRunner(void* arg) {
      auto creatorData = static_cast<zim::writer::CreatorData*>(arg);
      Task* task;
      unsigned int wait = 0;

      while(true) {
        microsleep(wait);
        wait += 100;
        if (creatorData->taskList.popFromQueue(task)) {
          if (task == nullptr) {
            return nullptr;
          }
          task->run(creatorData);
          delete task;
          wait = 0;
        }
      }
      return nullptr;
    }

    void* clusterWriter(void* arg) {
      auto creatorData = static_cast<zim::writer::CreatorData*>(arg);
      Cluster* cluster;
      unsigned int wait = 0;
      while(true) {
        microsleep(wait);
        wait += 100;
        if(creatorData->clusterToWrite.getHead(cluster)) {
          if (cluster == nullptr) {
            // All cluster writen, we can quit
            return nullptr;
          }
          if (not cluster->isClosed()) {
            continue;
          }
          creatorData->clusterToWrite.popFromQueue(cluster);
          cluster->setOffset(offset_t(lseek(creatorData->out_fd, 0, SEEK_CUR)));
          cluster->write(creatorData->out_fd);
          cluster->clear_data();
          wait = 0;
        }
      }
      return nullptr;
    }
  }
}
