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
#include "direntreader.h"
#include <zim/zim.h>
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

  bool DirentReader::initDirent(Dirent& dirent, const BufType& direntData) const
  {
    BufDataStream bds(direntData.data(), direntData.size());
    uint16_t mimeType = bds.read<uint16_t>();
    bool redirect = (mimeType == Dirent::redirectMimeType);
    bool linktarget = (mimeType == Dirent::linktargetMimeType);
    bool deleted = (mimeType == Dirent::deletedMimeType);
    uint8_t extraLen = bds.read<uint8_t>();
    char ns = bds.read<char>();
    uint32_t version = bds.read<uint32_t>();
    dirent.setVersion(version);

    if (redirect)
    {
      article_index_t redirectIndex(bds.read<article_index_type>());

      log_debug("redirectIndex=" << redirectIndex);

      dirent.setRedirect(article_index_t(redirectIndex));
    }
    else if (linktarget || deleted)
    {
      log_debug("linktarget or deleted entry");
      dirent.setArticle(mimeType, cluster_index_t(0), blob_index_t(0));
    }
    else
    {
      log_debug("read article entry");

      uint32_t clusterNumber = bds.read<uint32_t>();
      uint32_t blobNumber = bds.read<uint32_t>();

      log_debug("mimeType=" << mimeType << " clusterNumber=" << clusterNumber << " blobNumber=" << blobNumber);

      dirent.setArticle(mimeType, cluster_index_t(clusterNumber), blob_index_t(blobNumber));
    }

    std::string url;
    std::string title;
    std::string parameter;

    log_debug("read url, title and parameters");

    offset_type url_size = strnlen(bds.data(), bds.size() - extraLen);
    if (url_size >= bds.size())
      return false;

    url = bds.readString(url_size);
    bds.skip(1);

    offset_type title_size = strnlen(bds.data(), bds.size() - extraLen);
    if (title_size >= bds.size())
      return false;

    title = bds.readString(title_size);
    bds.skip(1);

    if (extraLen > bds.size())
      return false;

    parameter = bds.readString(extraLen);

    dirent.setUrl(ns, url);
    dirent.setTitle(title);
    dirent.setParameter(parameter);
    return true;
  }

  std::shared_ptr<const Dirent> DirentReader::readDirent(offset_t offset)
  {
    // We don't know the size of the dirent because it depends of the size of
    // the title, url and extra parameters.
    // This is a pity but we have no choice.
    // We cannot take a buffer of the size of the file, it would be really
    // inefficient. Let's do try, catch and retry while chosing a smart value
    // for the buffer size. Most dirent will be "Article" entry (header's size
    // == 16) without extra parameters. Let's hope that url + title size will
    // be < 256 and if not try again with a bigger size.

    size_t bufferSize(std::min(256UL, zimReader_->size().v-offset.v));
    auto dirent = std::make_shared<Dirent>();
    std::lock_guard<std::mutex> lock(bufferMutex_);
    for ( ; ; bufferSize += 256 ) {
      buffer_.resize(bufferSize);
      zimReader_->read(buffer_.data(), offset, zsize_t(bufferSize));
      if ( initDirent(*dirent, buffer_) )
        return dirent;
    }
  }

  std::string Dirent::getLongUrl() const
  {
    log_trace("Dirent::getLongUrl()");
    log_debug("namespace=" << getNamespace() << " title=" << getTitle());

    return std::string(1, getNamespace()) + '/' + getUrl();
  }

}
