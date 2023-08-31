/*
 * Copyright (C) 2020 Matthieu Gautier <mgautier@kymeria.fr>
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

#include <zim/writer/contentProvider.h>

#include "../fs.h"

const zim::size_type BUFFER_SIZE(1024*1024);

namespace zim
{
  namespace writer
  {
    Blob StringProvider::feed()
    {
      if (feeded) {
        return Blob(nullptr, 0);
      }
      feeded = true;
      return Blob(content.data(), content.size());
    }

    Blob SharedStringProvider::feed()
    {
      if (feeded) {
        return Blob(nullptr, 0);
      }
      feeded = true;
      return Blob(content->data(), content->size());
    }

    FileProvider::FileProvider(const std::string& filepath)
      : filepath(filepath),
        buffer(new char[BUFFER_SIZE]),
        fd(new DEFAULTFS::FD(DEFAULTFS::openFile(filepath))),
        offset(0)
    {
      size = fd->getSize().v;
    }

    FileProvider::~FileProvider() = default;

    Blob FileProvider::feed()
    {
      auto sizeToRead = std::min(BUFFER_SIZE, size-offset);
      if (!sizeToRead) {
        return Blob(nullptr, 0);
      }

      if(fd->readAt(buffer.get(), zim::zsize_t(sizeToRead), zim::offset_t(offset)) == zim::zsize_t(-1)) {
        throw std::runtime_error("Error reading file " + filepath);
      }
      offset += sizeToRead;
      return Blob(buffer.get(), sizeToRead);
    }
  }
}
