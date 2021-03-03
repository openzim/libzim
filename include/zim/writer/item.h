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
      COMPRESS,
      FRONT_ARTICLE,
    };
    using Hints = std::map<HintKeys, uint64_t>;

    class ContentProvider;
    class IndexData {
      public:
        using GeoPosition = std::tuple<bool, double, double>;
        virtual ~IndexData() = default;
        virtual bool hasIndexData() const = 0;
        virtual std::string getTitle() const = 0;
        virtual std::string getContent() const = 0;
        virtual std::string getKeywords() const = 0;
        virtual uint32_t getWordCount() const = 0;
        virtual GeoPosition getGeoPosition() const = 0;
    };

    /**
     * Item represent data to be added to the archive.
     *
     * This is a abstract class the user need to implement.
     * libzim provides `BasicItem`, `StringItem` and `FileItem`
     * to simplify (or avoid) this reimplementation.
     */
    class Item
    {
      public:
        /**
         * The path of the item.
         *
         * The path must be absolute.
         * Path must be unique.
         *
         * @return the path of the item.
         */
        virtual std::string getPath() const = 0;

        /**
         * The title of the item.
         *
         * Item's title is indexed and is used for the suggestion system.
         * Title don't have to be unique.
         *
         * @return the title of the item.
         */
        virtual std::string getTitle() const = 0;

        /**
         * The mimetype of the item.
         *
         * Mimetype is store within the content.
         * It is also used to detect if the content must be compressed or not.
         *
         * @return the mimetype of the item.
         */
        virtual std::string getMimeType() const = 0;

        /**
         * The content provider of the item.
         *
         * The content provider is responsible to provide the content to the creator.
         * The returned content provider must stay valid even after creator release
         * its reference to the item.
         *
         * This method will be called once by libzim, in the main thread
         * (but will be used in a different thread).
         * The default IndexData will also call this method once (more)
         * in the main thread (and use it in another thread).
         *
         * @return the contentProvider of the item.
         */
        virtual std::unique_ptr<ContentProvider> getContentProvider() const = 0;

        /**
         * The index data of the item.
         *
         * The index data is the data to index. (May be different from the content
         * to store).
         * The returned index data must stay valid even after creator release
         * its reference to the item.
         *
         * This method will be called twice by libzim if it is compiled with xapian
         * (and is configured to index data). You may return the same indexData.
         * The default implementation will use a contentProvider to get the content to index.
         * The contentProvider will be created in the main thread but the data reading and
         * parsing will occur in a different thread.
         *
         * @return the indexData of the item.
         *         May return a nullptr if there is no indexData.
         */
        virtual std::shared_ptr<IndexData> getIndexData() const;

        /**
         * Hints to help the creator takes decision about the item.
         *
         * @return A list of hints.
         */
        virtual Hints getHints() const;
        virtual ~Item() = default;

      private:
        mutable std::shared_ptr<IndexData> mp_defaultIndexData;
    };

    /**
     * A BasicItem is a partial implementation of a Item.
     *
     * `BasicItem` provides a basic implementation for everything about an `Item`
     * but the actual content of the item.
     */
    class BasicItem : public Item
    {
      public:
        /**
         * Create a BasicItem with the given path, mimetype and title.
         *
         * @param path the path of the item.
         * @param mimetype the mimetype of the item.
         * @param title the title of the item.
         */
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

    /**
     * A `StringItem` is a full implemented item where the content is stored in a string.
     */
    class StringItem : public BasicItem, public std::enable_shared_from_this<StringItem>
    {
      public:
        /**
         * Create a StringItem with the given path, mimetype, title and content.
         *
         * The parameters are the ones of the private constructor.
         *
         * @param path the path of the item.
         * @param mimetype the mimetype of the item.
         * @param title the title of the item.
         * @param content the content of the item.
         */
        template<typename... Ts>
        static std::shared_ptr<StringItem> create(Ts&&... params) {
          return std::shared_ptr<StringItem>(new StringItem(std::forward<Ts>(params)...));
        }

        std::unique_ptr<ContentProvider> getContentProvider() const;

      protected:
        std::string content;

      private:
        StringItem(const std::string& path, const std::string& mimetype,
                   const std::string& title, const std::string& content)
          : BasicItem(path, mimetype, title),
            content(content)
        {}



    };

    /**
     * A `FileItem` is a full implemented item where the content is file.
     */
    class FileItem : public BasicItem
    {
      public:
        /**
         * Create a FileItem with the given path, mimetype, title and filenpath.
         *
         * @param path the path of the item.
         * @param mimetype the mimetype of the item.
         * @param title the title of the item.
         * @param filepath the path of the file in the filesystem.
         */
        FileItem(const std::string& path, const std::string& mimetype,
                 const std::string& title, const std::string& filepath)
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
