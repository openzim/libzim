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

#include <zim/item.h>
#include "_dirent.h"
#include "cluster.h"
#include "fileimpl.h"
#include "file_part.h"
#include "log.h"

log_define("zim.item")

using namespace zim;

Item::Item(std::shared_ptr<FileImpl> file, entry_index_type idx)
  : m_file(file),
    m_idx(idx),
    m_dirent(file->getDirent(entry_index_t(idx)))
{}

std::string Item::getTitle() const
{
  return m_dirent->getTitle();
}

std::string Item::getPath() const
{
  if (m_file->hasNewNamespaceScheme()) {
    return m_dirent->getUrl();
  } else {
    return m_dirent->getLongUrl();
  }
}

std::string Item::getMimetype() const
{
  return m_file->getMimeType(m_dirent->getMimeType());
}

Blob Item::getData(offset_type offset) const
{
  auto size = getSize()-offset;
  return getData(offset, size);
}

Blob Item::getData(offset_type offset, size_type size) const
{
  auto cluster = m_file->getCluster(m_dirent->getClusterNumber());
  return cluster->getBlob(m_dirent->getBlobNumber(),
                          offset_t(offset),
                          zsize_t(size));
}

size_type Item::getSize() const
{
  auto cluster = m_file->getCluster(m_dirent->getClusterNumber());
  return size_type(cluster->getBlobSize(m_dirent->getBlobNumber()));
}

std::pair<std::string, offset_type> Item::getDirectAccessInformation() const
{
  auto cluster = m_file->getCluster(m_dirent->getClusterNumber());
  if (cluster->isCompressed()) {
    return std::make_pair("", 0);
  }

  auto full_offset = m_file->getBlobOffset(m_dirent->getClusterNumber(),
                                         m_dirent->getBlobNumber());

  auto part_its = m_file->getFileParts(full_offset, zsize_t(getSize()));
  auto first_part = part_its.first;
  if (++part_its.first != part_its.second) {
   // The content is split on two parts. We cannot have direct access
    return std::make_pair("", 0);
  }
  auto range = first_part->first;
  auto part = first_part->second;
  const offset_type local_offset(full_offset - range.min);
  const offset_type offset = local_offset + m_file->getArchiveStartOffset().v;
  return std::make_pair(part->filename(), offset);
}
