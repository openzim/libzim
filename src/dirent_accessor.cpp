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

#include "file_reader.h"
#include "_dirent.h"

using namespace zim;

DirectDirentAccessor::DirectDirentAccessor(std::shared_ptr<FileReader> zimReader, std::unique_ptr<const Reader> urlPtrReader, entry_index_t direntCount)
  : mp_zimReader(zimReader),
    mp_urlPtrReader(std::move(urlPtrReader)),
    m_direntCount(direntCount),
    m_bufferDirentZone(256)
{

}

std::shared_ptr<const Dirent> DirectDirentAccessor::getDirent(entry_index_t idx) const
{
  auto direntOffset = getOffset(idx);
  return readDirent(direntOffset);
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
  // We don't know the size of the dirent because it depends of the size of
  // the title, url and extra parameters.
  // This is a pitty but we have no choices.
  // We cannot take a buffer of the size of the file, it would be really inefficient.
  // Let's do try, catch and retry while chosing a smart value for the buffer size.
  // Most dirent will be "Article" entry (header's size == 16) without extra parameters.
  // Let's hope that url + title size will be < 256 and if not try again with a bigger size.
  std::shared_ptr<const Dirent> dirent;
  {
    std::lock_guard<std::mutex> l(m_bufferDirentLock);
    zsize_t bufferSize = zsize_t(256);
    // On very small file, the offset + 256 is higher than the size of the file,
    // even if the file is valid.
    // So read only to the end of the file.
    auto totalSize = mp_zimReader->size();
    if (offset.v + 256 > totalSize.v) bufferSize = zsize_t(totalSize.v-offset.v);
    while (true) {
        m_bufferDirentZone.reserve(size_type(bufferSize));
        mp_zimReader->read(m_bufferDirentZone.data(), offset, bufferSize);
        auto direntBuffer = Buffer::makeBuffer(m_bufferDirentZone.data(), bufferSize);
        try {
          dirent = std::make_shared<const Dirent>(direntBuffer);
        } catch (InvalidSize&) {
          // buffer size is not enougth, try again :
          bufferSize += 256;
          continue;
        }
        // Success !
        break;
    }
  }

  return dirent;
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
