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
#include <cxxtools/log.h>
#include <algorithm>
#include <stdint.h>
#include <zim/endian.h>

log_define("zim.dirent")

namespace zim
{
  //////////////////////////////////////////////////////////////////////
  // Dirent
  //

  std::ostream& operator<< (std::ostream& out, const Dirent& dirent)
  {
    char header[12];
    header[0] = static_cast<char>(dirent.isRedirect());
    header[1] = static_cast<char>(dirent.getMimeType());
    header[2] = '\0';
    header[3] = dirent.getNamespace();

    log_debug("title=" << dirent.getTitle() << " title.size()=" << dirent.getTitle().getValue().size() << " extralen=" << dirent.getExtraLen());

    if (dirent.isRedirect())
    {
      toLittleEndian(dirent.getRedirectIndex(), header + 4);
      toLittleEndian(dirent.getExtraLen(), header + 8);
      out.write(header, 10);
    }
    else
    {
      toLittleEndian(dirent.getClusterNumber(), header + 4);
      toLittleEndian(dirent.getBlobNumber(), header + 8);
      toLittleEndian(dirent.getExtraLen(), header + 12);
      out.write(header, 14);
    }

    out << dirent.getTitle().getValue();
    if (!dirent.getParameter().empty())
      out << '\0' << dirent.getParameter();

    return out;
  }

  std::istream& operator>> (std::istream& in, Dirent& dirent)
  {
    char header[14];
    in.read(header, 10);
    if (in.fail())
    {
      log_warn("error reading dirent header");
      return in;
    }

    if (in.gcount() != 10)
    {
      log_warn("error reading dirent header (2)");
      in.setstate(std::ios::failbit);
      return in;
    }

    bool redirect = header[0];
    char ns = header[3];
    size_type extraLen;
    if (redirect)
    {
      log_debug("read redirect entry");

      size_type redirectIndex = fromLittleEndian(reinterpret_cast<const size_type*>(header + 4));
      extraLen = fromLittleEndian(reinterpret_cast<const uint16_t*>(header + 8));

      log_debug("redirectIndex=" << redirectIndex << " extraLen=" << extraLen);

      dirent.setRedirect(redirectIndex);
    }
    else
    {
      log_debug("read article entry");

      in.read(header + 10, 4);
      if (in.fail())
      {
        log_warn("error reading article dirent header");
        return in;
      }

      if (in.gcount() != 4)
      {
        log_warn("error reading article dirent header (2)");
        return in;
        in.setstate(std::ios::failbit);
        return in;
      }

      MimeType mimeType = static_cast<MimeType>(header[1]);
      size_type clusterNumber = fromLittleEndian(reinterpret_cast<const size_type*>(header + 4));
      size_type blobNumber = fromLittleEndian(reinterpret_cast<const size_type*>(header + 8));
      extraLen = fromLittleEndian(reinterpret_cast<const uint16_t*>(header + 12));

      log_debug("mimeType=" << mimeType << " clusterNumber=" << clusterNumber << " blobNumber=" << blobNumber << " extraLen=" << extraLen);

      dirent.setArticle(mimeType, clusterNumber, blobNumber);
    }
    
    char ch;
    std::string title;
    std::string parameter;

    log_debug("read title and parameters; extraLen=" << extraLen);

    title.reserve(extraLen);
    while (extraLen && in.get(ch) && ch != '\0')
    {
      title += ch;
      --extraLen;
    }

    if (in && extraLen)
    {
      --extraLen;
      parameter.reserve(extraLen);
      while (extraLen-- && in.get(ch))
        parameter += ch;
    }

    dirent.setTitle(ns, QUnicodeString(title));
    dirent.setParameter(parameter);

    return in;
  }

}
