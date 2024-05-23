/*
 * Copyright (C) 2020 Veloman Yunkan
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

#ifndef ZIM_DIRENTREADER_H
#define ZIM_DIRENTREADER_H

#include "_dirent.h"
#include "reader.h"

#include <memory>
#include <mutex>
#include <vector>

namespace zim
{

// Unlke FileReader and MemoryReader (which read data from a file and memory,
// respectively), DirentReader is a helper class that reads Dirents (rather
// than from a Dirent).
class LIBZIM_API DirentReader
{
public: // functions
  explicit DirentReader(std::shared_ptr<const Reader> zimReader)
    : mp_zimReader(zimReader)
  {}

  std::shared_ptr<const Dirent> readDirent(offset_t offset);

private: // functions
  bool initDirent(Dirent& dirent, const Buffer& direntData) const;
  
private: // data
  std::shared_ptr<const Reader> mp_zimReader;
  std::vector<char> m_buffer;
  std::mutex m_bufferMutex;
};

} // namespace zim

#endif // ZIM_DIRENTREADER_H
