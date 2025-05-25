/*
 * Copyright (C) 2021 Veloman Yunkan
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

#define ZIM_PRIVATE
#include <zim/item.h>
#include "cluster.h"
#include "zim/zim.h"
#include "fileimpl.h"
#include "log.h"

#include <cassert>

log_define("zim.item")

using namespace zim;

Item::Item(const Entry& entry)
  : Entry(entry)
{
  assert(!entry.isRedirect());
}

std::string Item::getMimetype() const
{
  return m_file->getMimeType(m_dirent->getMimeType());
}

Blob Item::getData(offset_type offset) const
{
  return m_file->getBlob(*m_dirent, offset_t(offset));
}

Blob Item::getData(offset_type offset, size_type size) const
{
  return m_file->getBlob(*m_dirent, offset_t(offset), zsize_t(size));
}

size_type Item::getSize() const
{
  auto cluster = m_file->getCluster(m_dirent->getClusterNumber());
  return size_type(cluster->getBlobSize(m_dirent->getBlobNumber()));
}

ItemDataDirectAccessInfo Item::getDirectAccessInformation() const
{
  return m_file->getDirectAccessInformation(m_dirent->getClusterNumber(), m_dirent->getBlobNumber());
}

cluster_index_type Item::getClusterIndex() const
{
  return m_dirent->getClusterNumber().v;
}

blob_index_type Item::getBlobIndex() const
{
  return m_dirent->getBlobNumber().v;
}
