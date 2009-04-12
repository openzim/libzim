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
      case zimMimeTextHtml:
        return textHtml;
      case zimMimeTextPlain:
        return textPlain;
      case zimMimeImageJpeg:
        return imageJpeg;
      case zimMimeImagePng:
        return imagePng;
      case zimMimeImageTiff:
        return imageTiff;
      case zimMimeTextCss:
        return textCss;
      case zimMimeImageGif:
        return imageGif;
      case zimMimeIndex:
        return index;
      case zimMimeApplicationJavaScript:
        return applicationJavaScript;
      case zimMimeImageIcon:
        return imageIcon;
      case zimMimeTextXml:
        return textXml;
    }

    return textHtml;
  }

  size_type Article::getArticleSize() const
  {
    Dirent dirent = getDirent();
    return file.getCluster(dirent.getClusterNumber())
               .getBlobSize(dirent.getBlobNumber());
  }

}
