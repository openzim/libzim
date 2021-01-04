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
#include "debug.h"

#include <memory>
#include <mutex>
#include <vector>

namespace zim
{

class Dirent;
class Reader;
class FileReader;

/**
 * DirentAccessor is used to access a dirent from its index.
 * It doesn't provide any "advanced" features like lookup, find or cache.
 *
 * This is the base class to locate a dirent (offset) and read it.
 */

class DirentAccessor
{
public: // functions
  DirentAccessor(std::shared_ptr<FileReader> zimReader, std::unique_ptr<const Reader> urlPtrReader, entry_index_t direntCount);

  offset_t    getOffset(entry_index_t idx) const;
  std::shared_ptr<const Dirent> getDirent(entry_index_t idx) const;
  entry_index_t getDirentCount() const  {  return m_direntCount; }

private: // functions
  std::shared_ptr<const Dirent> readDirent(offset_t) const;

private: // data
  std::shared_ptr<FileReader>    mp_zimReader;
  std::unique_ptr<const Reader>  mp_urlPtrReader;
  entry_index_t                  m_direntCount;

  mutable std::vector<char>  m_bufferDirentZone;
  mutable std::mutex         m_bufferDirentLock;
};

} // namespace zim

#endif // ZIM_DIRENT_ACCESSOR_H
