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
#include "bufferstreamer.h"
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

  Dirent::Dirent(const Buffer& bufferRef)
    : Dirent()
  {
    BufferStreamer reader(bufferRef);
    uint16_t mimeType = reader.read<uint16_t>();
    bool redirect = (mimeType == Dirent::redirectMimeType);
    bool linktarget = (mimeType == Dirent::linktargetMimeType);
    bool deleted = (mimeType == Dirent::deletedMimeType);
    uint8_t extraLen = reader.read<uint8_t>();
    char ns = reader.read<char>();
    uint32_t version = reader.read<uint32_t>();
    setVersion(version);


    if (redirect)
    {
      article_index_t redirectIndex(reader.read<article_index_type>());

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

      uint32_t clusterNumber = reader.read<uint32_t>();
      uint32_t blobNumber = reader.read<uint32_t>();

      log_debug("mimeType=" << mimeType << " clusterNumber=" << clusterNumber << " blobNumber=" << blobNumber);

      setArticle(mimeType, cluster_index_t(clusterNumber), blob_index_t(blobNumber));
    }

    std::string url;
    std::string title;
    std::string parameter;

    log_debug("read url, title and parameters");

    size_type url_size = strnlen(
      reader.current(),
      reader.left().v - extraLen
    );
    if (url_size >= reader.left().v) {
      throw(InvalidSize());
    }
    url = std::string(reader.current(), url_size);
    reader.skip(zsize_t(url_size+1));

    size_type title_size = strnlen(
      reader.current(),
      reader.left().v - extraLen
    );
    if (title_size >= reader.left().v) {
      throw(InvalidSize());
    }
    title = std::string(reader.current(), title_size);
    reader.skip(zsize_t(title_size+1));

    if (extraLen > reader.left().v) {
       throw(InvalidSize());
    }
    parameter = std::string(reader.current(), extraLen);

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
