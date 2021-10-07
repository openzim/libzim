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

#ifndef OPENZIM_LIBZIM_COUNTER_HANDLER_H
#define OPENZIM_LIBZIM_COUNTER_HANDLER_H

#include "handler.h"

#include <map>

namespace zim {
namespace writer {


class CounterHandler : public DirentHandler {
  public:
    typedef std::map<std::string, entry_index_type> Counter;

    explicit CounterHandler(CreatorData* data);
    virtual ~CounterHandler();

    void start() override;
    void stop() override;
    bool isCompressible() override { return true; }
    ContentProviders getContentProviders() const override;
    void handle(Dirent* dirent, std::shared_ptr<Item> item) override;
    void handle(Dirent* dirent, const Hints& hints) override;

  private:
    Dirents createDirents() const override;
    CreatorData* mp_creatorData;
    Counter m_mimetypeCounter;
};

}
}

#endif // OPENZIM_LIBZIM_COUNTER_HANDLER_H
