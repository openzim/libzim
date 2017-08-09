/*
 * Copyright (C) 2008 Tommi Maekitalo
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

#include <zim/fileheader.h>
#include <iostream>
#include <algorithm>
#include "log.h"
#include "endian_tools.h"

log_define("zim.file.header")

namespace zim
{
  const size_type Fileheader::zimMagic = 0x044d495a; // ="ZIM^d"
  const size_type Fileheader::zimVersion = 5;
  const size_type Fileheader::size = 80;

  std::ostream& operator<< (std::ostream& out, const Fileheader& fh)
  {
    char header[Fileheader::size];
    toLittleEndian(Fileheader::zimMagic, header);
    toLittleEndian(Fileheader::zimVersion, header + 4);
    std::copy(fh.getUuid().data, fh.getUuid().data + sizeof(Uuid), header + 8);
    toLittleEndian(fh.getArticleCount(), header + 24);
    toLittleEndian(fh.getClusterCount(), header + 28);
    toLittleEndian(fh.getUrlPtrPos(), header + 32);
    toLittleEndian(fh.getTitleIdxPos(), header + 40);
    toLittleEndian(fh.getClusterPtrPos(), header + 48);
    toLittleEndian(fh.getMimeListPos(), header + 56);
    toLittleEndian(fh.getMainPage(), header + 64);
    toLittleEndian(fh.getLayoutPage(), header + 68);
    toLittleEndian(fh.getChecksumPos(), header + 72);

    out.write(header, Fileheader::size);

    return out;
  }

  void Fileheader::read(std::shared_ptr<Buffer> buffer)
  {
    size_type magicNumber = fromLittleEndian(buffer->as<size_type>(0));
    if (magicNumber != Fileheader::zimMagic)
    {
      log_error("invalid magic number " << magicNumber << " found - "
          << Fileheader::zimMagic << " expected");
    }

    uint16_t version = fromLittleEndian(buffer->as<uint16_t>(4));
    if (version != static_cast<size_type>(Fileheader::zimVersion))
    {
      log_error("invalid zimfile version " << version << " found - "
          << Fileheader::zimVersion << " expected");
    }

    Uuid uuid;
    std::copy(buffer->data(8), buffer->data(24), uuid.data);
    setUuid(uuid);

    setArticleCount(fromLittleEndian(buffer->as<size_type>(24)));
    setClusterCount(fromLittleEndian(buffer->as<size_type>(28)));
    setUrlPtrPos(fromLittleEndian(buffer->as<offset_type>(32)));
    setTitleIdxPos(fromLittleEndian(buffer->as<offset_type>(40)));
    setClusterPtrPos(fromLittleEndian(buffer->as<offset_type>(48)));
    setMimeListPos(fromLittleEndian(buffer->as<offset_type>(56)));
    setMainPage(fromLittleEndian(buffer->as<size_type>(64)));
    setLayoutPage(fromLittleEndian(buffer->as<size_type>(68)));
    setChecksumPos(fromLittleEndian(buffer->as<offset_type>(72)));
  }

}
