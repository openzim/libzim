/*
 * Copyright (C) 2017-2020 Mattieu Gautier <mgautier@kymeria.fr>
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

#include "fileheader.h"
#include <zim/error.h>
#include <iostream>
#include <algorithm>
#include "log.h"
#include "endian_tools.h"
#include "reader.h"
#include "bufferstreamer.h"
#include "buffer.h"
#ifdef _WIN32
# include "io.h"
#else
# include "unistd.h"
# define _write(fd, addr, size) ::write((fd), (addr), (size))
#endif

log_define("zim.file.header")

namespace zim
{
  const uint32_t Fileheader::zimMagic = 0x044d495a; // ="ZIM^d"
  const uint16_t Fileheader::zimOldMajorVersion = 5;
  const uint16_t Fileheader::zimMajorVersion = 6;
  const uint16_t Fileheader::zimMinorVersion = 3;
  const offset_type Fileheader::size = 80; // This is also mimeListPos (so an offset)

  Fileheader::Fileheader()
    : majorVersion(zimMajorVersion),
      minorVersion(zimMinorVersion),
      articleCount(0),
      titleIdxPos(0),
      pathPtrPos(0),
      clusterCount(0),
      clusterPtrPos(0),
      mainPage(std::numeric_limits<entry_index_type>::max()),
      layoutPage(std::numeric_limits<entry_index_type>::max()),
      checksumPos(std::numeric_limits<offset_type>::max())
  {}

  void Fileheader::write(int out_fd) const
  {
    char header[Fileheader::size];
    toLittleEndian(Fileheader::zimMagic, header);
    toLittleEndian(getMajorVersion(), header + 4);
    toLittleEndian(getMinorVersion(), header + 6);
    std::copy(getUuid().data, getUuid().data + sizeof(Uuid), header + 8);
    toLittleEndian(getArticleCount(), header + 24);
    toLittleEndian(getClusterCount(), header + 28);
    toLittleEndian(getPathPtrPos(), header + 32);
    toLittleEndian(getTitleIdxPos(), header + 40);
    toLittleEndian(getClusterPtrPos(), header + 48);
    toLittleEndian(getMimeListPos(), header + 56);
    toLittleEndian(getMainPage(), header + 64);
    toLittleEndian(getLayoutPage(), header + 68);
    toLittleEndian(getChecksumPos(), header + 72);

    auto ret = _write(out_fd, header, Fileheader::size);
    if (ret != Fileheader::size) {
      std::cerr << "Error Writing" << std::endl;
      std::cerr << "Ret is " << ret << std::endl;
      perror("Error writing");
      throw std::runtime_error("Error writing");
    }
  }

  void Fileheader::read(const Reader& reader)
  {
    auto buffer = reader.get_buffer(offset_t(0), zsize_t(Fileheader::size));
    auto seqReader = BufferStreamer(buffer);
    uint32_t magicNumber = seqReader.read<uint32_t>();
    if (magicNumber != Fileheader::zimMagic)
    {
      log_error("invalid magic number " << magicNumber << " found - "
          << Fileheader::zimMagic << " expected");
      throw ZimFileFormatError("Invalid magic number");
    }

    uint16_t major_version = seqReader.read<uint16_t>();
    if (major_version != zimOldMajorVersion && major_version != zimMajorVersion)
    {
      log_error("invalid zimfile major version " << major_version << " found - "
          << Fileheader::zimMajorVersion << " expected");
      throw ZimFileFormatError("Invalid version");
    }
    setMajorVersion(major_version);

    setMinorVersion(seqReader.read<uint16_t>());

    Uuid uuid;
    std::copy(seqReader.current(), seqReader.current()+16, uuid.data);
    seqReader.skip(zsize_t(16));
    setUuid(uuid);

    setArticleCount(seqReader.read<uint32_t>());
    setClusterCount(seqReader.read<uint32_t>());
    setPathPtrPos(seqReader.read<uint64_t>());
    setTitleIdxPos(seqReader.read<uint64_t>());
    setClusterPtrPos(seqReader.read<uint64_t>());
    setMimeListPos(seqReader.read<uint64_t>());
    setMainPage(seqReader.read<uint32_t>());
    setLayoutPage(seqReader.read<uint32_t>());
    setChecksumPos(seqReader.read<uint64_t>());

    sanity_check();
  }

  bool Fileheader::hasTitleListingV0() const {
    return titleIdxPos != offset_type(-1);
  }

  void Fileheader::sanity_check() const {
    if (!!articleCount != !!clusterCount) {
      throw ZimFileFormatError("No article <=> No cluster");
    }

    if (mimeListPos != size && mimeListPos != 72) {
      throw ZimFileFormatError("mimelistPos must be 80.");
    }

    if (pathPtrPos < mimeListPos) {
      throw ZimFileFormatError("pathPtrPos must be > mimelistPos.");
    }

    if (hasTitleListingV0() && titleIdxPos < mimeListPos) {
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
