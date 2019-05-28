/*
 * Copyright (C) 2006 Tommi Maekitalo
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

#include "_dirent.h"
#include <zim/zim.h>
#include "buffer.h"
#include "endian_tools.h"
#include "log.h"
#include <algorithm>
#include <cstring>

log_define("zim.dirent")

namespace zim
{
  //////////////////////////////////////////////////////////////////////
  // Dirent
  //

  const uint16_t Dirent::redirectMimeType;
  const uint16_t Dirent::linktargetMimeType;
  const uint16_t Dirent::deletedMimeType;

  Dirent::Dirent(std::unique_ptr<Buffer> buffer)
    : Dirent()
  {
    uint16_t mimeType = buffer->as<uint16_t>(offset_t(0));
    bool redirect = (mimeType == Dirent::redirectMimeType);
    bool linktarget = (mimeType == Dirent::linktargetMimeType);
    bool deleted = (mimeType == Dirent::deletedMimeType);
    uint8_t extraLen = buffer->data()[2];
    char ns = buffer->data()[3];
    uint32_t version = buffer->as<uint32_t>(offset_t(4));
    setVersion(version);

    offset_t current = offset_t(8);

    if (redirect)
    {
      article_index_t redirectIndex(buffer->as<article_index_type>(current));
      current += sizeof(article_index_t);

      log_debug("redirectIndex=" << redirectIndex);

      setRedirect(article_index_t(redirectIndex));
    }
    else if (linktarget || deleted)
    {
      log_debug("linktarget or deleted entry");
      setArticle(mimeType, cluster_index_t(0), blob_index_t(0));
    }
    else
    {
      log_debug("read article entry");

      uint32_t clusterNumber = buffer->as<uint32_t>(current);
      current += sizeof(uint32_t);
      uint32_t blobNumber = buffer->as<uint32_t>(current);
      current += sizeof(uint32_t);

      log_debug("mimeType=" << mimeType << " clusterNumber=" << clusterNumber << " blobNumber=" << blobNumber);

      setArticle(mimeType, cluster_index_t(clusterNumber), blob_index_t(blobNumber));
    }

    std::string url;
    std::string title;
    std::string parameter;

    log_debug("read url, title and parameters");

    offset_type url_size = strnlen(
      buffer->data(current),
      buffer->size().v - current.v - extraLen
    );
    if (current.v + url_size >= buffer->size().v) {
      throw(InvalidSize());
    }
    url = std::string(buffer->data(current), url_size);
    current += url_size + 1;

    offset_type title_size = strnlen(
      buffer->data(current),
      buffer->size().v - current.v - extraLen
    );
    if (current.v + title_size >= buffer->size().v) {
      throw(InvalidSize());
    }
    title = std::string(buffer->data(current), title_size);
    current += title_size + 1;

    if (current.v + extraLen > buffer->size().v) {
       throw(InvalidSize());
    }
    parameter = std::string(buffer->data(current), extraLen);

    setUrl(ns, url);
    setTitle(title);
    setParameter(parameter);

  }

  std::string Dirent::getLongUrl() const
  {
    log_trace("Dirent::getLongUrl()");
    log_debug("namespace=" << getNamespace() << " title=" << getTitle());

    return std::string(1, getNamespace()) + '/' + getUrl();
  }

}
