/*
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

// This handler is in charge of handling titles.
// It will create the "classic" old V0 title listing (for ALL entries) but also
// the V1 title listing (for front article only).
class TitleListingHandler : public DirentHandler {
  public:
    explicit TitleListingHandler(CreatorData* data);
    virtual ~TitleListingHandler();

    void start() override;
    void stop() override;
    bool isCompressible() override { return false; }
    ContentProviders getContentProviders() const override;
    void handle(Dirent* dirent, std::shared_ptr<Item> item) override;
    void handle(Dirent* dirent, const Hints& hints) override;

  protected:
    Dirents createDirents() const override;
    CreatorData* mp_creatorData;
    Dirents m_handledDirents;
};
}
}

#endif // OPENZIM_LIBZIM_LISTING_HANDLER_H
