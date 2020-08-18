/*
 * Copyright (C) 2009 Tommi Maekitalo
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

#ifndef ZIM_WRITER_ITEM_H
#define ZIM_WRITER_ITEM_H

#include <stdexcept>
#include <zim/blob.h>
#include <zim/zim.h>
#include <zim/uuid.h>
#include <string>

namespace zim
{
  namespace writer
  {
    class IndexData {
      public:
        virtual ~IndexData() = default;
        virtual bool hasIndexData() const = 0;
        virtual std::string getTitle() const = 0;
        virtual std::string getContent() const = 0;
        virtual std::string getKeywords() const = 0;
        virtual uint32_t getWordCount() const = 0;
        virtual std::tuple<bool, double, double> getGeoPosition() const = 0;
    };

    class Item
    {
      public:
        virtual std::string getPath() const = 0;
        virtual std::string getTitle() const = 0;
        virtual std::string getMimeType() const = 0;
        virtual zim::size_type getSize() const = 0;
        virtual Blob getData() const = 0;
        virtual std::unique_ptr<IndexData> getIndexData() const;
        virtual std::string getFilename() const = 0;
        virtual ~Item() = default;
    };

  }
}

#endif // ZIM_WRITER_ITEM_H
