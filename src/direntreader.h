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
class DirentReader
{
public: // types
  typedef std::vector<char> BufType;
public: // functions
  explicit DirentReader(std::shared_ptr<const Reader> zimReader)
    : zimReader_(zimReader)
  {}

  std::shared_ptr<const Dirent> readDirent(offset_t offset);

private: // functions
  bool initDirent(Dirent& dirent, const BufType& direntData) const;

private: // data
  std::shared_ptr<const Reader> zimReader_;
  BufType buffer_;
  std::mutex bufferMutex_;
};

} // namespace zim

#endif // ZIM_DIRENTREADER_H
