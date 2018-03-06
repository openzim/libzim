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
#include <zim/error.h>
#include <iostream>
#include <algorithm>
#include "log.h"
#include "endian_tools.h"
#include "buffer.h"

log_define("zim.file.header")

namespace zim
{
  const uint32_t Fileheader::zimMagic = 0x044d495a; // ="ZIM^d"
  const uint16_t Fileheader::zimClassicMajorVersion = 5;
  const uint16_t Fileheader::zimExtendedMajorVersion = 6;
  const uint16_t Fileheader::zimMinorVersion = 0;
  const offset_type Fileheader::size = 80; // This is also mimeListPos (so an offset)

  std::ostream& operator<< (std::ostream& out, const Fileheader& fh)
  {
    char header[Fileheader::size];
    toLittleEndian(Fileheader::zimMagic, header);
    toLittleEndian(fh.getMajorVersion(), header + 4);
    toLittleEndian(fh.getMinorVersion(), header + 6);
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

  void Fileheader::read(std::shared_ptr<const Buffer> buffer)
  {
    uint32_t magicNumber = buffer->as<uint32_t>(offset_t(0));
    if (magicNumber != Fileheader::zimMagic)
    {
      log_error("invalid magic number " << magicNumber << " found - "
          << Fileheader::zimMagic << " expected");
      throw ZimFileFormatError("Invalid magic number");
    }

    uint16_t major_version = buffer->as<uint16_t>(offset_t(4));
    if (major_version != zimClassicMajorVersion && major_version != zimExtendedMajorVersion)
    {
      log_error("invalid zimfile major version " << major_version << " found - "
          << Fileheader::zimMajorVersion << " expected");
      throw ZimFileFormatError("Invalid version");
    }
    setMajorVersion(major_version);

    setMinorVersion(buffer->as<uint16_t>(offset_t(6)));

    Uuid uuid;
    std::copy(buffer->data(offset_t(8)), buffer->data(offset_t(24)), uuid.data);
    setUuid(uuid);

    setArticleCount(buffer->as<uint32_t>(offset_t(24)));
    setClusterCount(buffer->as<uint32_t>(offset_t(28)));
    setUrlPtrPos(buffer->as<uint64_t>(offset_t(32)));
    setTitleIdxPos(buffer->as<uint64_t>(offset_t(40)));
    setClusterPtrPos(buffer->as<uint64_t>(offset_t(48)));
    setMimeListPos(buffer->as<uint64_t>(offset_t(56)));
    setMainPage(buffer->as<uint32_t>(offset_t(64)));
    setLayoutPage(buffer->as<uint32_t>(offset_t(68)));
    setChecksumPos(buffer->as<uint64_t>(offset_t(72)));

    sanity_check();
  }

  void Fileheader::sanity_check() const {
    if (!!articleCount != !!clusterCount) {
      throw ZimFileFormatError("No article <=> No cluster");
    }

    if (mimeListPos != size && mimeListPos != 72) {
      throw ZimFileFormatError("mimelistPos must be 80.");
    }

    if (urlPtrPos < mimeListPos) {
      throw ZimFileFormatError("urlPtrPos must be > mimelistPos.");
    }
    if (titleIdxPos < mimeListPos) {
      throw ZimFileFormatError("titleIdxPos must be > mimelistPos.");
    }
    if (clusterPtrPos < mimeListPos) {
      throw ZimFileFormatError("clusterPtrPos must be > mimelistPos.");
    }

    if (clusterCount > articleCount) {
      throw ZimFileFormatError("Cluster count cannot be higher than article count.");
    }

    if (checksumPos != 0 && checksumPos < mimeListPos) {
      throw ZimFileFormatError("checksumPos must be > mimeListPos.");
    }
  }

}
