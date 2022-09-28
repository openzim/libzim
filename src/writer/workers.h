/*
 * Copyright (C) 2019-2020 Matthieu Gautier <mgautier@kymeria.fr>
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

#ifndef OPENZIM_LIBZIM_WORKERS_H
#define OPENZIM_LIBZIM_WORKERS_H

#include "tools.h"
#include "creatordata.h"

namespace zim {
namespace writer {

class Task {
  public:
    Task() = default;
    virtual ~Task() = default;

    virtual void run(CreatorData* data) = 0;
};

template<class T>
class TrackableTask: public Task {
  public:
    TrackableTask(const TrackableTask&) = delete;
    TrackableTask& operator=(const TrackableTask&) = delete;
    TrackableTask() { ++waitingTaskCount; }
    virtual ~TrackableTask() { --waitingTaskCount;}

    static void waitNoMoreTask(const CreatorData* data) {
      // Wait for all tasks has been done
      // If we are in error state, threads have been stopped and waitingTaskCount
      // will never reach 0, so no need to wait.
      unsigned int wait = 0;
      do {
        microsleep(wait);
        wait += 10;
      } while(waitingTaskCount.load() > 0 && !data->isErrored());
    }

  private:
    static std::atomic<unsigned long> waitingTaskCount;
};

template<class T>
std::atomic<unsigned long> zim::writer::TrackableTask<T>::waitingTaskCount(0);

void* taskRunner(void* data);
void* clusterWriter(void* data);

}
}

#endif // OPENZIM_LIBZIM_WORKERS_H
