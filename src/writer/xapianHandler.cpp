/*
 * Copyright 2020 Matthieu Gautier <mgautier@kymeria.fr>
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

FullTextXapianHandler::FullTextXapianHandler(CreatorData* data)
  : mp_indexer(new XapianIndexer(data->zimName+"_fulltext.idx", data->indexingLanguage, IndexingMode::FULL, true)),
    mp_creatorData(data)
{}

FullTextXapianHandler::~FullTextXapianHandler() = default;

void FullTextXapianHandler::start() {
  mp_indexer->indexingPrelude();
}

void FullTextXapianHandler::stop() {
  // We need to wait that all indexation tasks have been done before closing the
  // xapian database.
  unsigned int wait = 0;
  do {
    microsleep(wait);
    wait += 10;
  } while (IndexTask::waiting_task.load() > 0);
  mp_indexer->indexingPostlude();
}

Dirent* FullTextXapianHandler::createDirent() const {
  return mp_creatorData->createDirent('X', "fulltext/xapian", "application/octet-stream+xapian", "");
}

std::unique_ptr<ContentProvider> FullTextXapianHandler::getContentProvider() const {
  return std::unique_ptr<ContentProvider>(new FileProvider(mp_indexer->getIndexPath()));
}

void FullTextXapianHandler::handle(Dirent* dirent, const Hints& hints)
{
  // We have nothing to do.
}

void FullTextXapianHandler::handle(Dirent* dirent, std::shared_ptr<Item> item)
{
  if (dirent->getNamespace() != 'C') {
    // We should always have namespace == 'C' but let's be careful.
    return;
  }
  mp_creatorData->taskList.pushToQueue(new IndexTask(item, mp_indexer.get()));
}

TitleXapianHandler::TitleXapianHandler(CreatorData* data)
  : mp_indexer(new XapianIndexer(data->zimName+"_title.idx", data->indexingLanguage, IndexingMode::TITLE, true)),
    mp_creatorData(data)
{}

TitleXapianHandler::~TitleXapianHandler() = default;

void TitleXapianHandler::start() {
  mp_indexer->indexingPrelude();
}

void TitleXapianHandler::stop() {
  mp_indexer->indexingPostlude();
}

Dirent* TitleXapianHandler::createDirent() const {
  return mp_creatorData->createDirent('X', "title/xapian", "application/octet-stream+xapian", "");
}

std::unique_ptr<ContentProvider> TitleXapianHandler::getContentProvider() const {
  return std::unique_ptr<ContentProvider>(new FileProvider(mp_indexer->getIndexPath()));
}

void TitleXapianHandler::handle(Dirent* dirent, const Hints& hints)
{
  // We have no items to get the title from. So it is a redirect
  // We assume that if the redirect has a title, we must index it.
  if (dirent->getNamespace() != 'C') {
    // We should always have namespace == 'C' but let's be careful.
    return;
  }

  auto title = dirent->getRealTitle();
  if (title.empty()) {
    return;
  }
  auto path = dirent->getPath();
  mp_indexer->indexTitle(path, title);
}

void TitleXapianHandler::handle(Dirent* dirent, std::shared_ptr<Item> item)
{
  // We have a item. And items have indexData. We must use it.
  if (dirent->getNamespace() != 'C') {
    // We should always have namespace == 'C' but let's be careful.
    return;
  }

  auto indexData = item->getIndexData();
  if (!indexData || !indexData->hasIndexData()) {
    return;
  }
  auto title = indexData->getTitle();
  if (title.empty()) {
    return;
  }
  auto path = dirent->getPath();
  mp_indexer->indexTitle(path, title);
}

