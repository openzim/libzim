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

#include <zim/blob.h>
#include <zim/writer/articlesource.h>

namespace zim
{
  namespace writer
  {
    size_type Article::getVersion() const
    {
      return 0;
    }

    bool Article::isRedirect() const
    {
      return false;
    }

    bool Article::isLinktarget() const
    {
      return false;
    }

    bool Article::isDeleted() const
    {
      return false;
    }

    bool Article::shouldCompress() const
    {
      std::string mimeType = getMimeType();
      return mimeType != "image/jpeg"
          && mimeType != "image/png"
          && mimeType != "image/tiff"
          && mimeType != "image/gif"
          && mimeType != "application/zip";
    }

    std::string Article::getRedirectAid() const
    {
      return std::string();
    }

    std::string Article::getParameter() const
    {
      return std::string();
    }

    std::string Article::getNextCategory()
    {
      return std::string();
    }

    /**************************************************************************/
    /* For API compatibility.
     * The default Article::getData call ArticleSource::getData.
     * This should be removed once every users switch to new API.
     */
    Blob Article::getData() const
    {
      std::cerr << "DEPRECATED WARNING : Use of ArticleSource::getData is deprecated." << std::endl;
      std::cerr << "                     You should override Article::getData directly." << std::endl;
      return __source->getData(getAid());
    }
    /**************************************************************************/

    Uuid ArticleSource::getUuid()
    {
      return Uuid::generate();
    }

    std::string ArticleSource::getMainPage()
    {
      return std::string();
    }

    std::string ArticleSource::getLayoutPage()
    {
      return std::string();
    }

    Category* ArticleSource::getCategory(const std::string& cid)
    {
      return 0;
    }

  }
}
