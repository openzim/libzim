/*
 * Copyright (C) 2021 Maneesh P M <manu.pm55@gmail.com>
 * Copyright (C) 2020-2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#include "xapianHandler.h"
#include "xapianIndexer.h"
#include "xapianWorker.h"
#include "creatordata.h"

#include <zim/writer/contentProvider.h>

using namespace zim::writer;

XapianHandler::XapianHandler(CreatorData* data, bool withFulltextIndex)
  : mp_fulltextIndexer(withFulltextIndex ? new XapianIndexer(data->zimName+"_fulltext.idx", data->indexingLanguage, IndexingMode::FULL, true) : nullptr),
    mp_titleIndexer(new XapianIndexer(data->zimName+"_title.idx", data->indexingLanguage, IndexingMode::TITLE, true)),
    mp_creatorData(data)
{}

XapianHandler::~XapianHandler() = default;

void XapianHandler::start() {
  if (mp_fulltextIndexer) {
    mp_fulltextIndexer->indexingPrelude();
  }
  mp_titleIndexer->indexingPrelude();
}

void XapianHandler::stop() {
  // We need to wait that all indexation tasks have been done before closing the
  // xapian database.
  if (mp_fulltextIndexer) {
    IndexTask::waitNoMoreTask();
    mp_fulltextIndexer->indexingPostlude();
  }
  mp_titleIndexer->indexingPostlude();
}

DirentHandler::Dirents XapianHandler::createDirents() const {
  // Wait for all task to be done before checking if we are empty.
  Dirents ret;
  if (mp_fulltextIndexer) {
    IndexTask::waitNoMoreTask();
    if (!mp_fulltextIndexer->is_empty()) {
      ret.push_back(mp_creatorData->createDirent(NS::X, "fulltext/xapian", "application/octet-stream+xapian", ""));
    }
  }
  if (!mp_titleIndexer->is_empty()) {
    ret.push_back(mp_creatorData->createDirent(NS::X, "title/xapian", "application/octet-stream+xapian", ""));
  }
  return ret;
}

DirentHandler::ContentProviders XapianHandler::getContentProviders() const {
  ContentProviders ret;
  if (mp_fulltextIndexer && !mp_fulltextIndexer->is_empty()) {
    ret.push_back(std::unique_ptr<ContentProvider>(new FileProvider(mp_fulltextIndexer->getIndexPath())));
  }
  if (!mp_titleIndexer->is_empty()) {
    ret.push_back(std::unique_ptr<ContentProvider>(new FileProvider(mp_titleIndexer->getIndexPath())));
  }
  return ret;
}

void XapianHandler::indexTitle(Dirent* dirent) {
  auto title = dirent->getRealTitle();
  if (title.empty()) {
    return;
  }
  auto path = dirent->getPath();
  if (dirent->isRedirect()) {
    auto redirectPath = dirent->getRedirectPath();
    mp_titleIndexer->indexTitle(path, title, redirectPath);
  } else {
    mp_titleIndexer->indexTitle(path, title);
  }
}

void XapianHandler::handle(Dirent* dirent, const Hints& hints)
{
  if (dirent->getNamespace() != NS::C) {
    return;
  }

  try {
    if (bool(hints.at(FRONT_ARTICLE))) {
      indexTitle(dirent);
    }
  } catch(std::out_of_range&) {}
}

void XapianHandler::handle(Dirent* dirent, std::shared_ptr<Item> item)
{
  if (dirent->getNamespace() != NS::C) {
    return;
  }

  // Title index.
  handle(dirent, item->getAmendedHints());

  // FullText index
  if (mp_fulltextIndexer) {
    auto indexData = item->getIndexData();
    if (!indexData) {
      return;
    }
    auto title = indexData->getTitle();
    auto path = dirent->getPath();
    mp_creatorData->taskList.pushToQueue(new IndexTask(indexData, path, title, mp_fulltextIndexer.get()));
  }
}

