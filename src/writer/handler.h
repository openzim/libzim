/*
 * Copyright 2020 Matthieu Gautier <mgautier@kymeria.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU  General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef OPENZIM_LIBZIM_WRITER_HANDLER_H
#define OPENZIM_LIBZIM_WRITER_HANDLER_H

#include <string>
#include <memory>

#include <zim/writer/item.h>

namespace zim {
namespace writer {

class CreatorData;
class ContentProvider;
class Dirent;

/**
 * DirentHandler is used to add "extra" handling on dirent/item.
 *
 * The main purpose of the handle is to "see" all dirents corresponding to user entries
 * and generate it's own dirent/item.
 *
 * Classical use cases are :
 *  - Generating a index of the item (xapianIndex)
 *  - Generating a listing of the item (all item or "main" entries only)
 *  - Count mimetypes
 *  - ...
 *
 *  While it seems that DirentHandler is dynamically (de)activated by user it is not.
 *  This is purelly a internal structur to simplify the internal architecture of the writer.
 */
class DirentHandler {
  public:
    explicit DirentHandler(CreatorData* data);
    virtual ~DirentHandler() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual Dirent* getDirent() const = 0;
    virtual std::unique_ptr<ContentProvider> getContentProvider() const = 0;

    /*
     * Handle a dirent/item.
     *
     * item may be nullptr (dirent is a redirect or in special case)
     */
    virtual void handle(Dirent* dirent, const Hints& hints, std::shared_ptr<Item> item) = 0;

  protected:
    DirentHandler() = default;
};

}
}

#endif // OPENZIM_LIBZIM_WRITER_HANDLER_H
