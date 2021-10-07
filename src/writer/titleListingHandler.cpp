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

#include "titleListingHandler.h"
#include "creatordata.h"

#include "../endian_tools.h"

#include <zim/writer/contentProvider.h>
#include <zim/blob.h>

using namespace zim::writer;

namespace {

class ListingProvider : public ContentProvider {
  public:
    ListingProvider(const TitleListingHandler::Dirents* dirents, bool frontOnly)
      : mp_dirents(dirents),
        m_it(dirents->begin()),
        m_frontOnly(frontOnly)
    {}

    zim::size_type getSize() const override {
      if (m_frontOnly) {
        auto nbFrontArticles = std::count_if(mp_dirents->begin(), mp_dirents->end(), [](Dirent* d) { return d->isFrontArticle();});
        return nbFrontArticles * sizeof(zim::entry_index_type);
      } else {
        return mp_dirents->size() * sizeof(zim::entry_index_type);
      }
    }

    zim::Blob feed() override {
      if (m_frontOnly) {
        while (m_it != mp_dirents->end() && !(*m_it)->isFrontArticle()) {
          m_it++;
        }
      }
      if (m_it == mp_dirents->end()) {
        return zim::Blob(nullptr, 0);
      }
      zim::toLittleEndian((*m_it)->getIdx().v, buffer);
      m_it++;
      return zim::Blob(buffer, sizeof(zim::entry_index_type));
    }

  private:
    const TitleListingHandler::Dirents* mp_dirents;
    char buffer[sizeof(zim::entry_index_type)];
    TitleListingHandler::Dirents::const_iterator m_it;
    bool m_frontOnly;
};

} // end of anonymous namespace

TitleListingHandler::TitleListingHandler(CreatorData* data)
  : mp_creatorData(data),
    m_hasFrontArticles(false)
{}

TitleListingHandler::~TitleListingHandler() = default;

void TitleListingHandler::start() {
}

void TitleListingHandler::stop() {
  m_handledDirents.erase(
    std::remove_if(m_handledDirents.begin(), m_handledDirents.end(), [](const Dirent* d) { return d->isRemoved(); }),
    m_handledDirents.end());
  std::sort(m_handledDirents.begin(), m_handledDirents.end(), TitleCompare());
}

DirentHandler::Dirents TitleListingHandler::createDirents() const {
  Dirents ret;
  ret.push_back(mp_creatorData->createDirent(NS::X, "listing/titleOrdered/v0", "application/octet-stream+zimlisting", ""));
  if (m_hasFrontArticles) {
    ret.push_back(mp_creatorData->createDirent(NS::X, "listing/titleOrdered/v1", "application/octet-stream+zimlisting", ""));
  }
  return ret;
}

DirentHandler::ContentProviders TitleListingHandler::getContentProviders() const {
  ContentProviders ret;
  ret.push_back(std::unique_ptr<ContentProvider>(new ListingProvider(&m_handledDirents, false)));
  if (m_hasFrontArticles) {
    ret.push_back(std::unique_ptr<ContentProvider>(new ListingProvider(&m_handledDirents, true)));
  }
  return ret;
}

void TitleListingHandler::handle(Dirent* dirent, std::shared_ptr<Item> item)
{
  handle(dirent, item->getHints());
}

void TitleListingHandler::handle(Dirent* dirent, const Hints& hints)
{
  m_handledDirents.push_back(dirent);
  try {
    if(bool(hints.at(FRONT_ARTICLE))) {
      m_hasFrontArticles = true;
      dirent->setFrontArticle();
    }
  } catch(std::out_of_range&) {}
}

