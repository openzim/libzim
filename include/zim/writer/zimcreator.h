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

#ifndef ZIM_WRITER_ZIMCREATOR_H
#define ZIM_WRITER_ZIMCREATOR_H

#include <memory>
#include <zim/zim.h>
#include <zim/writer/article.h>

namespace zim
{
  class Fileheader;
  namespace writer
  {
    class ZimCreatorData;
    class ZimCreator
    {
      private:
        std::unique_ptr<ZimCreatorData> data;
        void fillHeader(Fileheader* header);
        void write(const Fileheader& header, const std::string& fname) const;
        bool verbose;
        bool withIndex;
        size_t minChunkSize;
        std::string indexingLanguage;


      public:
        ZimCreator(bool verbose = false);
        virtual ~ZimCreator();

        zim::size_type getMinChunkSize() const;
        void setMinChunkSize(zim::size_type s);
        void setIndexing(bool indexing, std::string language);

        virtual void startZimCreation(const std::string& fname);
        virtual void addArticle(const Article& article);
        virtual void finishZimCreation();

        virtual std::string getMainPage();
        virtual std::string getLayoutPage();
        virtual zim::Uuid getUuid();
    };

  }

}

#endif // ZIM_WRITER_ZIMCREATOR_H
