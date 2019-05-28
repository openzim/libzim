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

#ifndef ZIM_WRITER_ARTICLESOURCE_H
#define ZIM_WRITER_ARTICLESOURCE_H

#include <stdexcept>
#include <zim/blob.h>
#include <zim/zim.h>
#include <zim/uuid.h>
#include <zim/writer/url.h>
#include <string>

namespace zim
{
  namespace writer
  {
    class ArticleSource;
    class Article
    {
      public:
        virtual Url getUrl() const = 0;
        virtual std::string getTitle() const = 0;
        virtual bool isRedirect() const = 0;
        virtual bool isLinktarget() const;
        virtual bool isDeleted() const;
        virtual std::string getMimeType() const = 0;
        virtual bool shouldCompress() const = 0;
        virtual bool shouldIndex() const = 0;
        virtual Url getRedirectUrl() const = 0;
        virtual zim::size_type getSize() const = 0;
        virtual Blob getData() const = 0;
        virtual std::string getFilename() const = 0;
        virtual ~Article() = default;

        // returns the next category id, to which the article is assigned to
        virtual std::string getNextCategory();
    };

    class Category
    {
      public:
        virtual Blob getData() = 0;
        virtual std::string getUrl() const = 0;
        virtual std::string getTitle() const = 0;
        virtual ~Category() = default;
    };

    class ArticleSource
    {
      public:
        virtual void setFilename(const std::string& fname) { }
        virtual const Article* getNextArticle() = 0;

        // After fetching the articles and for each article the category ids
        // using Article::getNextCategory, the writer has a list of category
        // ids. Using this list, the writer fetches the category data using
        // this method.
        virtual Category* getCategory(const std::string& cid);
        virtual ~ArticleSource() = default;
    };

  }
}

#endif // ZIM_WRITER_ARTICLESOURCE_H
