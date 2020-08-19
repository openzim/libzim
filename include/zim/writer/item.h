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

#include <map>

namespace zim
{
  namespace writer
  {
    enum HintKeys {
      COMPRESS
    };

    class ContentProvider;
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
        using Hints = std::map<HintKeys, uint64_t>;
        virtual std::string getPath() const = 0;
        virtual std::string getTitle() const = 0;
        virtual std::string getMimeType() const = 0;
        virtual std::unique_ptr<ContentProvider> getContentProvider() const = 0;
        virtual std::unique_ptr<IndexData> getIndexData() const;
        virtual Hints getHints() const;
        virtual ~Item() = default;
    };

    class BasicItem : public Item
    {
      public:
        BasicItem(const std::string& path, const std::string& mimetype, const std::string& title)
          : path(path),
            mimetype(mimetype),
            title(title)
        {}

        std::string getPath() const { return path; }
        std::string getTitle() const { return title; }
        std::string getMimeType() const { return mimetype; }

      protected:
        std::string path;
        std::string mimetype;
        std::string title;
    };

    class StringItem : public BasicItem, std::enable_shared_from_this<StringItem>
    {
      public:
        StringItem(const std::string& path, const std::string& mimetype, const std::string& title, const std::string& content)
          : BasicItem(path, mimetype, title),
            content(content)
        {}

        std::unique_ptr<ContentProvider> getContentProvider() const;
        std::unique_ptr<IndexData> getIndexData() const;

      protected:
        std::string content;

    };

    class FileItem : public BasicItem
    {
      public:
        FileItem(const std::string& path, const std::string& mimetype, const std::string& title, const std::string& filepath)
          : BasicItem(path, mimetype, title),
            filepath(filepath)
        {}

        std::unique_ptr<ContentProvider> getContentProvider() const;

      protected:
        std::string filepath;
    };

  }
}

#endif // ZIM_WRITER_ITEM_H
