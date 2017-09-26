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

#include <zim/dirent.h>
#include <zim/zim.h>
#include <zim/buffer.h>
#include <zim/endian_tools.h>
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

  std::ostream& operator<< (std::ostream& out, const Dirent& dirent)
  {
    union
    {
      char d[16];
      long a;
    } header;
    toLittleEndian(dirent.getMimeType(), header.d);
    header.d[2] = static_cast<char>(dirent.getParameter().size());
    header.d[3] = dirent.getNamespace();

    log_debug("title=" << dirent.getTitle() << " title.size()=" << dirent.getTitle().size());

    toLittleEndian(dirent.getVersion(), header.d + 4);

    if (dirent.isRedirect())
    {
      toLittleEndian(dirent.getRedirectIndex(), header.d + 8);
      out.write(header.d, 12);
    }
    else if (dirent.isLinktarget() || dirent.isDeleted())
    {
      out.write(header.d, 8);
    }
    else
    {
      toLittleEndian(dirent.getClusterNumber(), header.d + 8);
      toLittleEndian(dirent.getBlobNumber(), header.d + 12);
      out.write(header.d, 16);
    }

    out << dirent.getUrl() << '\0';

    std::string t = dirent.getTitle();
    if (t != dirent.getUrl())
      out << t;
    out << '\0' << dirent.getParameter();

    return out;
  }

  Dirent::Dirent(std::unique_ptr<Buffer> buffer)
  {
    uint16_t mimeType = buffer->as<uint16_t>(0);
    bool redirect = (mimeType == Dirent::redirectMimeType);
    bool linktarget = (mimeType == Dirent::linktargetMimeType);
    bool deleted = (mimeType == Dirent::deletedMimeType);
    uint8_t extraLen = buffer->data()[2];
    char ns = buffer->data()[3];
    size_type version = buffer->as<size_type>(4);
    setVersion(version);

    offset_type current = 8;

    if (redirect)
    {
      size_type redirectIndex = buffer->as<size_type>(current);
      current += sizeof(size_type);

      log_debug("redirectIndex=" << redirectIndex);

      setRedirect(redirectIndex);
    }
    else if (linktarget || deleted)
    {
      log_debug("linktarget or deleted entry");
      setArticle(mimeType, 0, 0);
    }
    else
    {
      log_debug("read article entry");

      size_type clusterNumber = buffer->as<size_type>(current);
      current += sizeof(size_type);
      size_type blobNumber = buffer->as<size_type>(current);
      current += sizeof(size_type);

      log_debug("mimeType=" << mimeType << " clusterNumber=" << clusterNumber << " blobNumber=" << blobNumber);

      setArticle(mimeType, clusterNumber, blobNumber);
    }
    
    std::string url;
    std::string title;
    std::string parameter;

    log_debug("read url, title and parameters");

    offset_type url_size = strlen(buffer->data(current));
    if (current + url_size >= buffer->size()) {
      throw(InvalidSize());
    }
    url = std::string(buffer->data(current), url_size);
    current += url_size + 1;
    
    offset_type title_size = strlen(buffer->data(current));
    if (current + title_size >= buffer->size()) {
      throw(InvalidSize());
    }
    title = std::string(buffer->data(current), title_size);
    current += title_size + 1;

    if (current + extraLen > buffer->size()) {
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
