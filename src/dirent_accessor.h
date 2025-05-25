/*
 * Copyright (C) 2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#ifndef ZIM_DIRENT_ACCESSOR_H
#define ZIM_DIRENT_ACCESSOR_H

#include "zim_types.h"
#include "lrucache.h"
#include "config.h"

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace zim
{

class Dirent;
class Reader;
class DirentReader;

/**
 * DirectDirentAccessor is used to access a dirent from its index.
 * It doesn't provide any "advanced" features like lookup or find.
 *
 * This is the base class to locate a dirent (offset) and read it.
 *
 */

class LIBZIM_PRIVATE_API DirectDirentAccessor
{
public: // functions
  DirectDirentAccessor(std::shared_ptr<DirentReader> direntReader,
                       std::unique_ptr<const Reader> pathPtrReader,
                       entry_index_t direntCount);

  offset_t    getOffset(entry_index_t idx) const;
  std::shared_ptr<const Dirent> getDirent(entry_index_t idx) const;
  entry_index_t getDirentCount() const  {  return m_direntCount; }

  size_t getMaxCacheSize() const { return m_direntCache.getMaxCost(); }
  size_t getCurrentCacheSize() const { return m_direntCache.cost(); }
  void setMaxCacheSize(size_t nbDirents) const { m_direntCache.setMaxCost(nbDirents); }

private: // functions
  std::shared_ptr<const Dirent> readDirent(offset_t) const;

private: // data
  std::shared_ptr<DirentReader>  mp_direntReader;
  std::unique_ptr<const Reader>  mp_pathPtrReader;
  entry_index_t                  m_direntCount;

  mutable lru_cache<entry_index_type, std::shared_ptr<const Dirent>, UnitCostEstimation> m_direntCache;
  mutable std::mutex m_direntCacheLock;

  mutable std::vector<char>  m_bufferDirentZone;
  mutable std::mutex         m_bufferDirentLock;
};

class IndirectDirentAccessor
{
  public:
    IndirectDirentAccessor(std::shared_ptr<const DirectDirentAccessor>, std::unique_ptr<const Reader> indexReader, title_index_t direntCount);

    entry_index_t getDirectIndex(title_index_t idx) const;
    std::shared_ptr<const Dirent> getDirent(title_index_t idx) const;
    title_index_t getDirentCount() const { return m_direntCount; }

  private: // data
    std::shared_ptr<const DirectDirentAccessor> mp_direntAccessor;
    std::unique_ptr<const Reader>               mp_indexReader;
    title_index_t                               m_direntCount;
};

} // namespace zim

#endif // ZIM_DIRENT_ACCESSOR_H
