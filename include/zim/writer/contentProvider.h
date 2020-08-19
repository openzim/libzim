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

#ifndef ZIM_WRITER_CONTENTPROVIDER_H
#define ZIM_WRITER_CONTENTPROVIDER_H

#include <stdexcept>
#include <zim/blob.h>
#include <zim/zim.h>
#include <string>

namespace zim
{
  namespace writer
  {
    class ContentProvider {
      public:
        virtual ~ContentProvider() = default;
        virtual zim::size_type getSize() const = 0;
        virtual Blob feed() = 0;
    };

    class StringProvider : public ContentProvider {
      public:
        explicit StringProvider(const std::string& content)
          : content(content),
            feeded(false)
        {}
        zim::size_type getSize() const { return content.size(); }
        Blob feed();

      protected:
        std::string content;
        bool feeded;
    };

    class SharedStringProvider : public ContentProvider {
      public:
        explicit SharedStringProvider(std::shared_ptr<const std::string> content)
          : content(content),
            feeded(false)
        {}
        zim::size_type getSize() const { return content->size(); }
        Blob feed();

      protected:
        std::shared_ptr<const std::string> content;
        bool feeded;

    };

    class FileProvider : public ContentProvider {
      public:
        explicit FileProvider(const std::string& filename);
        ~FileProvider();
        zim::size_type getSize() const { return size; }
        Blob feed();

      protected:
        int fd;
        zim::size_type size;
        std::unique_ptr<char[]> buffer;
    };

  }
}

#endif // ZIM_WRITER_CONTENTPROVIDER_H
