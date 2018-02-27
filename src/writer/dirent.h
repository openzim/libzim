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

#ifndef ZIM_WRITER_DIRENT_H
#define ZIM_WRITER_DIRENT_H

#include "../dirent.h"

namespace zim
{
  namespace writer
  {
    class Dirent : public zim::Dirent
    {
        std::string aid;
        std::string redirectAid;
        article_index_t idx;
        bool compress;

      public:
        Dirent()
          : idx(0)
          {}

        Dirent(const std::string& aid_)
          : aid(aid_),
            idx(0)
          {}

        Dirent(char ns, const std::string& url)
          : idx(0)
          { setUrl(ns, url); }

        void setAid(const std::string&  aid_)      { aid = aid_; }
        const std::string& getAid() const          { return aid; }

        void setRedirectAid(const std::string&  aid_)     { redirectAid = aid_; }
        const std::string& getRedirectAid() const         { return redirectAid; }

        void setIdx(article_index_t idx_)      { idx = idx_; }
        article_index_t getIdx() const         { return idx; }

        void setCompress(bool sw = true)  { compress = sw; }
        bool isCompress() const           { return compress; }
    };

    inline bool compareUrl(const Dirent& d1, const Dirent& d2)
    {
      return d1.getNamespace() < d2.getNamespace()
         || (d1.getNamespace() == d2.getNamespace()
           && d1.getUrl() < d2.getUrl());
    }

    inline bool compareAid(const Dirent& d1, const Dirent& d2)
    {
      return d1.getAid() < d2.getAid();
    }

  }
}

#endif // ZIM_WRITER_DIRENT_H

