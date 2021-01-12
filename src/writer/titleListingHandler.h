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

#ifndef OPENZIM_LIBZIM_LISTING_HANDLER_H
#define OPENZIM_LIBZIM_LISTING_HANDLER_H

#include "handler.h"
#include "_dirent.h"

#include <vector>

namespace zim {
namespace writer {

struct TitleCompare {
  bool operator() (const Dirent* d1, const Dirent* d2) const {
    return compareTitle(d1, d2);
  }
};

class TitleListingHandler : public DirentHandler {
  public:
    typedef std::vector<Dirent*> Dirents;

    explicit TitleListingHandler(CreatorData* data);
    virtual ~TitleListingHandler();

    void start() override;
    void stop() override;
    std::unique_ptr<ContentProvider> getContentProvider() const override;
    void handle(Dirent* dirent, const Hints& hints, std::shared_ptr<Item> item) override;

  protected:
    Dirent* getDirent() const override;
    CreatorData* mp_creatorData;
    Dirents m_dirents;
};

class TitleListingHandlerV1 : public TitleListingHandler {
  public:
    explicit TitleListingHandlerV1(CreatorData* data) : TitleListingHandler(data) {};
    void handle(Dirent* dirent, const Hints& hints, std::shared_ptr<Item> item) override;

  protected:
    Dirent* getDirent() const override;
};

}
}

#endif // OPENZIM_LIBZIM_LISTING_WORKER_H
