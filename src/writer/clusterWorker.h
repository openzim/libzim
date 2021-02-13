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

#ifndef OPENZIM_LIBZIM_CLUSTER_WORKER_H
#define OPENZIM_LIBZIM_CLUSTER_WORKER_H

#include <atomic>
#include "workers.h"

namespace zim {
namespace writer {

class Cluster;

class ClusterTask : public Task {
  public:
    ClusterTask(const ClusterTask&) = delete;
    ClusterTask& operator=(const ClusterTask&) = delete;
    explicit ClusterTask(Cluster* cluster) :
      cluster(cluster)
    {
      ++waiting_task;
    };
    virtual ~ClusterTask()
    {
      --waiting_task;
    }

    virtual void run(CreatorData* data);
    static std::atomic<unsigned long> waiting_task;

  private:
    Cluster* cluster;
};

}
}

#endif // OPENZIM_LIBZIM_QUEUE_H
