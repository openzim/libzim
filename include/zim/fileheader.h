/*
 * Copyright (C) 2008 Tommi Maekitalo
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

#ifndef ZIM_FILEHEADER_H
#define ZIM_FILEHEADER_H

#include <memory>
#include "zim.h"
#include "uuid.h"
#include <iosfwd>
#include <limits>

#ifdef _WIN32
#define NOMINMAX 1
#include <windows.h>
#undef NOMINMAX
#undef max
#endif

namespace zim
{
  class Buffer;
  class Fileheader
  {
    public:
      static const uint32_t zimMagic;
      static const uint16_t zimClassicMajorVersion;
      static const uint16_t zimExtendedMajorVersion;
      static const uint16_t zimMinorVersion;
      static const size_type size;

    private:
      uint16_t majorVersion;
      uint16_t minorVersion;
      Uuid uuid;
      article_index_type articleCount;
      offset_type titleIdxPos;
      offset_type urlPtrPos;
      offset_type mimeListPos;
      cluster_index_type clusterCount;
      offset_type clusterPtrPos;
      article_index_type mainPage;
      article_index_type layoutPage;
      offset_type checksumPos;

    public:
      Fileheader()
        : majorVersion(zimClassicMajorVersion),
          minorVersion(zimMinorVersion),
          articleCount(0),
          titleIdxPos(0),
          urlPtrPos(0),
          clusterCount(0),
          clusterPtrPos(0),
          mainPage(std::numeric_limits<article_index_type>::max()),
          layoutPage(std::numeric_limits<article_index_type>::max()),
          checksumPos(std::numeric_limits<offset_type>::max())
      {}

      void read(std::shared_ptr<const Buffer> buffer);

      // Do some sanity check, raise a ZimFileFormateError is
      // something is wrong.
      void sanity_check() const;

      uint16_t getMajorVersion() const             { return majorVersion; }
      void setMajorVersion(uint16_t v)             { majorVersion = v; }

      uint16_t getMinorVersion() const             { return minorVersion; }
      void setMinorVersion(uint16_t v)             { minorVersion = v; }

      const Uuid& getUuid() const                  { return uuid; }
      void setUuid(const Uuid& uuid_)              { uuid = uuid_; }

      article_index_type getArticleCount() const            { return articleCount; }
      void      setArticleCount(article_index_type s)       { articleCount = s; }

      offset_type getTitleIdxPos() const           { return titleIdxPos; }
      void        setTitleIdxPos(offset_type p)    { titleIdxPos = p; }

      offset_type getUrlPtrPos() const             { return urlPtrPos; }
      void        setUrlPtrPos(offset_type p)      { urlPtrPos = p; }

      offset_type getMimeListPos() const           { return mimeListPos; }
      void        setMimeListPos(offset_type p)    { mimeListPos = p; }

      cluster_index_type   getClusterCount() const          { return clusterCount; }
      void        setClusterCount(cluster_index_type s)     { clusterCount = s; }

      offset_type getClusterPtrPos() const         { return clusterPtrPos; }
      void        setClusterPtrPos(offset_type p)  { clusterPtrPos = p; }

      bool        hasMainPage() const              { return mainPage != std::numeric_limits<article_index_type>::max(); }
      article_index_type   getMainPage() const     { return mainPage; }
      void        setMainPage(article_index_type s){ mainPage = s; }

      bool        hasLayoutPage() const            { return layoutPage != std::numeric_limits<article_index_type>::max(); }
      article_index_type   getLayoutPage() const   { return layoutPage; }
      void        setLayoutPage(article_index_type s)       { layoutPage = s; }

      bool        hasChecksum() const              { return getMimeListPos() >= 80; }
      offset_type getChecksumPos() const           { return hasChecksum() ? checksumPos : 0; }
      void        setChecksumPos(offset_type p)    { checksumPos = p; }
  };

  std::ostream& operator<< (std::ostream& out, const Fileheader& fh);

}

#endif // ZIM_FILEHEADER_H
