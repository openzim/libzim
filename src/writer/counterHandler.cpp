/*
 * Copyright 2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#include "counterHandler.h"
#include "creatordata.h"

#include <zim/writer/contentProvider.h>
#include <zim/blob.h>

using namespace zim::writer;

CounterHandler::CounterHandler(CreatorData* data)
  : mp_creatorData(data)
{}

CounterHandler::~CounterHandler() = default;

void CounterHandler::start() {
}

void CounterHandler::stop() {
}

Dirent* CounterHandler::createDirent() const {
  return mp_creatorData->createDirent('M', "Counter", "text/plain", "");
}

std::unique_ptr<ContentProvider> CounterHandler::getContentProvider() const {
  std::stringstream ss;
  bool first = true;
  for(auto pair: m_mimetypeCounter) {
    if (! first) {
      ss << ";";
    }
    ss << pair.first << "=" << pair.second;
    first = false;
  }
  return std::unique_ptr<ContentProvider>(new StringProvider(ss.str()));
}

void CounterHandler::handle(Dirent* dirent, const Hints& hints)
{
}

void CounterHandler::handle(Dirent* dirent, std::shared_ptr<Item> item)
{
  if (dirent->getNamespace() != 'C') {
    return;
  }
  auto mimetype = item->getMimeType();
  if (mimetype.empty()) {
    return;
  }
  m_mimetypeCounter[mimetype] += 1;
}
