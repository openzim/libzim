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
#include <zim/blob.h>
#include "bufdatastream.h"
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

  Dirent::Dirent(const Blob& direntData)
    : Dirent()
  {
    BufDataStream bds(direntData.data(), direntData.size());
    uint16_t mimeType = bds.read<uint16_t>();
    bool redirect = (mimeType == Dirent::redirectMimeType);
    bool linktarget = (mimeType == Dirent::linktargetMimeType);
    bool deleted = (mimeType == Dirent::deletedMimeType);
    uint8_t extraLen = bds.read<uint8_t>();
    char ns = bds.read<char>();
    uint32_t version = bds.read<uint32_t>();
    setVersion(version);

    if (redirect)
    {
      article_index_t redirectIndex(bds.read<article_index_type>());

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

      uint32_t clusterNumber = bds.read<uint32_t>();
      uint32_t blobNumber = bds.read<uint32_t>();

      log_debug("mimeType=" << mimeType << " clusterNumber=" << clusterNumber << " blobNumber=" << blobNumber);

      setArticle(mimeType, cluster_index_t(clusterNumber), blob_index_t(blobNumber));
    }

    std::string url;
    std::string title;
    std::string parameter;

    log_debug("read url, title and parameters");

    offset_type url_size = strnlen(bds.data(), bds.size() - extraLen);
    if (url_size >= bds.size()) {
      throw(InvalidSize());
    }
    url = bds.readString(url_size);
    bds.skip(1);

    offset_type title_size = strnlen(bds.data(), bds.size() - extraLen);
    if (title_size >= bds.size()) {
      throw(InvalidSize());
    }
    title = bds.readString(title_size);
    bds.skip(1);

    if (extraLen > bds.size()) {
       throw(InvalidSize());
    }
    parameter = bds.readString(extraLen);

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
