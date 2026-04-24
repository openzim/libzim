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
#include "tools.h"

using namespace zim::writer;

XapianHandler::XapianHandler(CreatorData* data)
  : mp_fulltextIndexer(new XapianIndexer(data->zimName+"_fulltext.idx", data->indexingLanguage, IndexingMode::FULL, true)),
    mp_creatorData(data)
{}

XapianHandler::~XapianHandler() = default;


void XapianHandler::waitNoMoreTask() const {
  IndexTask::waitNoMoreTask(mp_creatorData);
}

void XapianHandler::start() {
  mp_fulltextIndexer->indexingPrelude();
}

void XapianHandler::stop() {
  // We need to wait that all indexation tasks have been done before closing the
  // xapian database.
  waitNoMoreTask();
  mp_fulltextIndexer->indexingPostlude();
}

DirentHandler::Dirents XapianHandler::createDirents() const {
  // Wait for all task to be done before checking if we are empty.
  Dirents ret;
  waitNoMoreTask();
  if (!mp_fulltextIndexer->is_empty()) {
    ret.push_back(mp_creatorData->createDirent(NS::X, "fulltext/xapian", "application/octet-stream+xapian", ""));
  }
  return ret;
}

DirentHandler::ContentProviders XapianHandler::getContentProviders() const {
  ContentProviders ret;
  if (!mp_fulltextIndexer->is_empty()) {
    ret.push_back(std::unique_ptr<ContentProvider>(new FileProvider(mp_fulltextIndexer->getIndexPath())));
  }
  return ret;
}

void XapianHandler::handle(const Dirent&)
{
}

void XapianHandler::handle(const Dirent& dirent, std::shared_ptr<Item> item)
{
  if (dirent.getNamespace() != NS::C) {
    return;
  }

  // Title index.
  handle(dirent);

  // FullText index
  auto indexData = item->getIndexData();
  if (!indexData) {
    return;
  }
  auto path = dirent.getPath();
  mp_creatorData->taskList.pushToQueue(std::make_shared<IndexTask>(indexData, path, mp_fulltextIndexer.get()));
}

