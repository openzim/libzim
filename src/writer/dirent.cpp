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

std::ostream& zim::writer::operator<< (std::ostream& out, const zim::writer::Dirent& dirent)
{
  union
  {
    char d[16];
    long a;
  } header;
  zim::toLittleEndian(dirent.getMimeType(), header.d);
  header.d[2] = 0; // parameter size
  header.d[3] = dirent.getNamespace();

  log_debug("title=" << dirent.getTitle() << " title.size()=" << dirent.getTitle().size());

  zim::toLittleEndian(dirent.getVersion(), header.d + 4);

  if (dirent.isRedirect())
  {
    zim::toLittleEndian(dirent.getRedirectIndex().v, header.d + 8);
    out.write(header.d, 12);
  }
  else if (dirent.isLinktarget() || dirent.isDeleted())
  {
    out.write(header.d, 8);
  }
  else
  {
    zim::toLittleEndian(zim::cluster_index_type(dirent.getClusterNumber()), header.d + 8);
    zim::toLittleEndian(zim::blob_index_type(dirent.getBlobNumber()), header.d + 12);
    out.write(header.d, 16);
  }

  out << dirent.getUrl() << '\0';

  std::string t = dirent.getTitle();
  if (t != dirent.getUrl())
    out << t;
  out << '\0';

  return out;
}
