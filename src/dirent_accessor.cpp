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

#include "dirent_accessor.h"

#include "direntreader.h"
#include "_dirent.h"
#include "envvalue.h"

#include <mutex>

#include <zim/error.h>

using namespace zim;

DirectDirentAccessor::DirectDirentAccessor(std::shared_ptr<DirentReader> direntReader, std::unique_ptr<const Reader> urlPtrReader, entry_index_t direntCount)
  : mp_direntReader(direntReader),
    mp_urlPtrReader(std::move(urlPtrReader)),
    m_direntCount(direntCount),
    m_direntCache(envValue("ZIM_DIRENTCACHE", DIRENT_CACHE_SIZE)),
    m_bufferDirentZone(256)
{}

std::shared_ptr<const Dirent> DirectDirentAccessor::getDirent(entry_index_t idx) const
{
  {
    std::lock_guard<std::mutex> l(m_direntCacheLock);
    auto v = m_direntCache.get(idx.v);
    if (v.hit()) {
      return v.value();
    }
  }

  auto direntOffset = getOffset(idx);
  auto dirent = readDirent(direntOffset);
  std::lock_guard<std::mutex> l(m_direntCacheLock);
  m_direntCache.put(idx.v, dirent);

  return dirent;
}

offset_t DirectDirentAccessor::getOffset(entry_index_t idx) const
{
  if (idx >= m_direntCount) {
    throw std::out_of_range("entry index out of range");
  }
  offset_t offset(mp_urlPtrReader->read_uint<offset_type>(offset_t(sizeof(offset_type)*idx.v)));
  return offset;
}

std::shared_ptr<const Dirent> DirectDirentAccessor::readDirent(offset_t offset) const
{
  return mp_direntReader->readDirent(offset);
}


IndirectDirentAccessor::IndirectDirentAccessor(std::shared_ptr<const DirectDirentAccessor> direntAccessor, std::unique_ptr<const Reader> indexReader, title_index_t direntCount)
  : mp_direntAccessor(direntAccessor),
    mp_indexReader(std::move(indexReader)),
    m_direntCount(direntCount)
{}

entry_index_t IndirectDirentAccessor::getDirectIndex(title_index_t idx) const
{
  if (idx >= m_direntCount) {
    throw std::out_of_range("entry index out of range");
  }
  entry_index_t index(mp_indexReader->read_uint<entry_index_type>(offset_t(sizeof(entry_index_t)*idx.v)));
  return index;
}

std::shared_ptr<const Dirent> IndirectDirentAccessor::getDirent(title_index_t idx) const
{
  auto directIndex = getDirectIndex(idx);
  return mp_direntAccessor->getDirent(directIndex);
}
