/*
 * Copyright (C) 2020 Matthieu Gautier <mgautier@kymeria.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * is provided AS IS, WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, and
 * NON-INFRINGEMENT.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#include "workers.h"
#include "cluster.h"
#include "creatordata.h"

#include "../tools.h"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace zim
{
  namespace writer
  {

    void* taskRunner(void* arg) {
      auto creatorData = static_cast<zim::writer::CreatorData*>(arg);
      Task* task;
      unsigned int wait = 0;

      while(true) {
        microsleep(wait);
        wait += 100;
        if (creatorData->taskList.popFromQueue(task)) {
          if (task == nullptr) {
            return nullptr;
          }
          if (creatorData->m_errored) {
            // If the creator is in error, stop the workers
            return nullptr;
          }
          try {
            task->run(creatorData);
          } catch (...) {
            std::lock_guard<std::mutex> l(creatorData->m_exceptionLock);
            creatorData->m_exceptionSlot = std::current_exception();
          }
          delete task;
          wait = 0;
        }
      }
      return nullptr;
    }

    void* clusterWriter(void* arg) {
      auto creatorData = static_cast<zim::writer::CreatorData*>(arg);
      Cluster* cluster;
      unsigned int wait = 0;
      while(true) {
        microsleep(wait);
        wait += 100;
        if(creatorData->clusterToWrite.getHead(cluster)) {
          if (cluster == nullptr) {
            // All cluster writen, we can quit
            return nullptr;
          }
          if (creatorData->m_errored) {
            // If the creator is in error, stop the workers
            return nullptr;
          }
          if (not cluster->isClosed()) {
            continue;
          }
          creatorData->clusterToWrite.popFromQueue(cluster);
          cluster->setOffset(offset_t(lseek(creatorData->out_fd, 0, SEEK_CUR)));
          cluster->write(creatorData->out_fd);
          cluster->clear_data();
          wait = 0;
        }
      }
      return nullptr;
    }
  }
}
