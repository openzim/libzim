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

log_define("zim.dirent")

namespace zim
{
  //////////////////////////////////////////////////////////////////////
  // Dirent
  //

  const size_type Dirent::headerSize;

  Dirent::Dirent()
  {
    std::fill(header, header + headerSize, '\0');
  }

  void Dirent::setExtra(const std::string& extra)
  {
    std::string::size_type p = extra.find('\0');
    if (p == std::string::npos)
    {
      title = extra;
      parameter.clear();
    }
    else
    {
      title.assign(extra, 0, p);
      parameter.assign(extra, p + 1, extra.size() - p - 1);
    }

    setExtraLen(extra.size());
  }

  std::ostream& operator<< (std::ostream& out, const Dirent& dirent)
  {
    out.write(dirent.header, Dirent::headerSize);
    out << dirent.title;
    if (!dirent.parameter.empty())
      out << '\0' << dirent.parameter;
    return out;
  }

  std::istream& operator>> (std::istream& in, Dirent& dirent)
  {
    in.read(dirent.header, Dirent::headerSize);
    if (in.gcount() != Dirent::headerSize)
      in.setstate(std::ios::failbit);
    else
    {
      std::string extra;
      char ch;
      uint16_t len = dirent.getExtraLen();
      extra.reserve(len);
      while (len-- && in.get(ch))
        extra += ch;
      dirent.setExtra(extra);
    }
    return in;
  }

}
