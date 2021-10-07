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
          pools.push_back(reinterpret_cast<Dirent*>(new char[sizeof(Dirent)*0xFFFF]));
          direntIndex = 0;
        }
        static void destroyPoolBlock(Dirent* pool, uint16_t count=0xFFFF) {
          for (auto i = 0U; i < count; i++) {
            try {
              pool[i].~Dirent();
            } catch (...){ /*discard*/ }
          }
          delete [] (reinterpret_cast<char*>(pool));
        }


      public:
        DirentPool() :
          direntIndex(0xFFFF)
        {}
        DirentPool(const DirentPool&) = delete;
        DirentPool& operator=(const DirentPool&) = delete;
        ~DirentPool() {
          auto nbPools = pools.size();
          if (nbPools == 0) {
            return;
          }
          // Delete all but last pools (add call the destructors of the dirents)
          for (auto i = 0U; i<nbPools-1; i++) {
            destroyPoolBlock(pools[i]);
          }
          // On the last pool, only `direntIndex` are really constructed.
          destroyPoolBlock(pools[nbPools-1], direntIndex);
        }

        Dirent* getClassicDirent(NS ns, const std::string& path, const std::string& title, uint16_t mimetype) {
          if (direntIndex == 0xFFFF) {
            allocate_new_pool();
          }
          auto dirent = pools.back() + direntIndex++;
          new (dirent) Dirent(ns, path, title, mimetype);
          return dirent;
        }

        Dirent* getRedirectDirent(NS ns, const std::string& path, const std::string& title, NS targetNs, const std::string& targetPath) {
          if (direntIndex == 0xFFFF) {
            allocate_new_pool();
          }
          auto dirent = pools.back() + direntIndex++;
          new (dirent) Dirent(ns, path, title, targetNs, targetPath);
          return dirent;
        }
    };
  }
}

#endif // ZIM_WRITER_DIRENTPOLL_H

