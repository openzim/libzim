/*
 * Copyright (C) 2006 Tommi Maekitalo
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

#include <zim/article.h>
#include <cxxtools/log.h>

log_define("zim.article")

namespace zim
{
  const std::string& Article::getMimeType() const
  {
    static const std::string textHtml = "text/html; charset=UTF-8";
    static const std::string textPlain = "text/plain";
    static const std::string textXml = "application/xml";
    static const std::string imageJpeg = "image/jpeg";
    static const std::string imagePng = "image/png";
    static const std::string imageTiff = "image/tiff";
    static const std::string textCss = "text/css";
    static const std::string imageGif = "image/gif";
    static const std::string index = "text/plain";
    static const std::string applicationJavaScript = "application/x-javascript";
    static const std::string imageIcon = "image/x-icon";

    switch (getLibraryMimeType())
    {
      case Dirent::zimMimeTextHtml:
        return textHtml;
      case Dirent::zimMimeTextPlain:
        return textPlain;
      case Dirent::zimMimeImageJpeg:
        return imageJpeg;
      case Dirent::zimMimeImagePng:
        return imagePng;
      case Dirent::zimMimeImageTiff:
        return imageTiff;
      case Dirent::zimMimeTextCss:
        return textCss;
      case Dirent::zimMimeImageGif:
        return imageGif;
      case Dirent::zimMimeIndex:
        return index;
      case Dirent::zimMimeApplicationJavaScript:
        return applicationJavaScript;
      case Dirent::zimMimeImageIcon:
        return imageIcon;
      case Dirent::zimMimeTextXml:
        return textXml;
    }

    return textHtml;
  }

  void Article::uncompressData() const
  {
    if (!getRedirectFlag() && uncompressedData.empty() && !getRawData().empty())
      uncompressedData = const_cast<File&>(file).uncompressData(dirent, getRawData());
  }

  std::string Article::getData() const
  {
    if (getRedirectFlag())
      return std::string();
    uncompressData();

    return getArticleSize() > 0 ? uncompressedData.substr(getArticleOffset(), getArticleSize())
                                : uncompressedData;
  }

  const std::string& Article::getRawData() const
  {
    log_trace("getRawData()");

    if (!dataRead)
    {
      Article* article = const_cast<Article*>(this);
      log_debug("read data from file");
      article->data.assign(
        const_cast<File&>(file).readData(getDirent().getOffset(),
        getDirent().getSize()));
      article->dataRead = true;
    }

    return data;
  }

  Article Article::getRedirectArticle() const
  {
    return const_cast<Article*>(this)->getFile().getArticle(getRedirectIndex());
  }

}
