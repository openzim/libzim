/*
 * Copyright (C) 2020-2021 Matthieu Gautier <mgautier@kymeria.fr>
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

#include <zim/writer/item.h>
#include <zim/writer/contentProvider.h>
#include "defaultIndexData.h"

namespace zim
{
  namespace writer
  {
    std::shared_ptr<IndexData> Item::getIndexData() const
    {
      if (getMimeType().find("text/html")!=0) {
        return nullptr;
      }

      auto provider = getContentProvider();
      return std::make_shared<DefaultIndexData>(std::move(provider), getTitle());
    }

    Hints Item::getHints() const {
      return Hints();
    }

    Hints Item::getAmendedHints() const {
      auto hints = getHints();

      // If not FRONT_ARTICLE hints is given, determine it from the mimetype.
      if (hints.find(FRONT_ARTICLE) == hints.end()) {
        hints[FRONT_ARTICLE] = (getMimeType().find("text/html") == 0);
      }

      // If not COMPRESS hints is given, determine it from the mimetype.
      if (hints.find(COMPRESS) == hints.end()) {
        hints[COMPRESS] = isCompressibleMimetype(getMimeType());
      }
      return hints;
    }

    std::unique_ptr<ContentProvider> StringItem::getContentProvider() const
    {
      auto shared_string = std::shared_ptr<const std::string>(shared_from_this(), &content);
      return std::unique_ptr<ContentProvider>(new SharedStringProvider(shared_string));
    }

    std::unique_ptr<ContentProvider> FileItem::getContentProvider() const
    {
      return std::unique_ptr<ContentProvider>(new FileProvider(filepath));
    }


  }
}
