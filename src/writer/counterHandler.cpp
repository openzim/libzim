/*
 * Copyright (C) 2021 Matthieu Gautier <mgautier@kymeria.fr>
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
#include <zim/tools.h>

using namespace zim::writer;

CounterHandler::CounterHandler(CreatorData* data)
  : mp_creatorData(data)
{}

CounterHandler::~CounterHandler() = default;

void CounterHandler::start() {
}

void CounterHandler::stop() {
}

DirentHandler::Dirents CounterHandler::createDirents() const {
  Dirents ret;
  ret.push_back(mp_creatorData->createDirent(NS::M, "Counter", "text/plain", ""));
  return ret;
}

DirentHandler::ContentProviders CounterHandler::getContentProviders() const {
  ContentProviders ret;
  Formatter fmt;
  bool first = true;
  for(auto pair: m_mimetypeCounter) {
    if (! first) {
      fmt << ";";
    }
    fmt << pair.first << "=" << pair.second;
    first = false;
  }
  ret.push_back(std::unique_ptr<ContentProvider>(new StringProvider(fmt)));
  return ret;
}

std::string CounterHandler::stripMimeParameters(const std::string& rawMimeType) {
  size_t pos = rawMimeType.find(';');

  // string is clean if no semicolon found
  if (pos == std::string::npos){
    return rawMimeType;
  }

  std::string cleanMime = rawMimeType.substr(0, pos);

  // removing whitespaces, might be optional
  size_t end = cleanMime.find_last_not_of(" \t");
  if (end != std::string::npos){
    cleanMime = cleanMime.substr(0, end+1);
  }
  else {
    // fallback if string is just empty space before the semicolon, really not sure about this
    cleanMime = "";
  }

  return cleanMime;
}


void CounterHandler::handle(Dirent* dirent, const Hints& hints)
{
}

void CounterHandler::handle(Dirent* dirent, std::shared_ptr<Item> item)
{
  if (dirent->getNamespace() != NS::C) {
    return;
  }
  auto mimetype = item->getMimeType();
  if (mimetype.empty()) {
    return;
  }

  // Strip mimetype before passing it as map key
  auto cleanMimetype = stripMimeParameters(mimetype);

  if (cleanMimetype.empty()){
    return;
  }

  m_mimetypeCounter[cleanMimetype] += 1;
}
