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

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

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

    FileProvider::FileProvider(const std::string& filename)
      : buffer(new char[1024*1024])
    {
      fd = open(filename.c_str(), O_RDONLY);
      if (fd == -1) {
        throw std::runtime_error(std::string("cannot open ") + filename);
      }
      size = lseek(fd, 0, SEEK_END);
      lseek(fd, 0, SEEK_SET);
    }

    FileProvider::~FileProvider()
    {
      if(fd != -1) {
        close(fd);
      }
    }

    Blob FileProvider::feed()
    {
      auto outSize = read(fd, buffer.get(), 1024*1024UL);
      return Blob(buffer.get(), outSize);
    }
  }
}
