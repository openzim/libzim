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

#ifndef OPENZIM_LIBZIM_XAPIAN_WORKER_H
#define OPENZIM_LIBZIM_XAPIAN_WORKER_H

#include <atomic>
#include <memory>
#include "workers.h"
#include <zim/writer/item.h>

namespace zim {
namespace writer {

class Item;
class XapianIndexer;

class IndexTask : public Task {
  public:
    IndexTask(const IndexTask&) = delete;
    IndexTask& operator=(const IndexTask&) = delete;
    IndexTask(std::shared_ptr<Item> item, XapianIndexer* indexer) :
      mp_indexData(item->getIndexData()),
      m_path(item->getPath()),
      m_title(item->getTitle()),
      mp_indexer(indexer)
    {
      ++waiting_task;
    }
    virtual ~IndexTask()
    {
      --waiting_task;
    }

    virtual void run(CreatorData* data);
    static std::atomic<unsigned long> waiting_task;

  private:
    std::shared_ptr<IndexData> mp_indexData;
    std::string m_path;
    std::string m_title;
    XapianIndexer* mp_indexer;
};

}
}

#endif // OPENZIM_LIBZIM_XAPIAN_WORKER_H
