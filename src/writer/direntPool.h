/*
 * Copyright (C) 2019 Matthieu Gautier
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

#ifndef ZIM_WRITER_DIRENTPOOL_H
#define ZIM_WRITER_DIRENTPOOL_H

#include "debug.h"
#include "_dirent.h"

namespace zim
{
  namespace writer {
    class DirentPool {
      private:
        std::vector<Dirent*> pools;
        uint16_t direntIndex;

        void allocate_new_pool() {
          pools.push_back(new Dirent[0xFFFF]);
          direntIndex = 0;
        }

      public:
        DirentPool() :
          direntIndex(0xFFFF)
        {}
        ~DirentPool() {
          for(auto direntArray: pools) {
            delete[] direntArray;
          }
        }

        Dirent* getDirent() {
          if (direntIndex == 0xFFFF) {
            allocate_new_pool();
          }
          return pools.back() + direntIndex++;
        }
    };
  }
}

#endif // ZIM_WRITER_DIRENTPOLL_H

