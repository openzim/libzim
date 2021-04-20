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

#ifndef OPENZIM_LIBZIM_XAPIAN_HANDLER_H
#define OPENZIM_LIBZIM_XAPIAN_HANDLER_H

#include "handler.h"

namespace zim {
namespace writer {

class XapianIndexer;

class FullTextXapianHandler : public DirentHandler {
  public:
    explicit FullTextXapianHandler(CreatorData* data);
    virtual ~FullTextXapianHandler();

    void start() override;
    void stop() override;
    bool isCompressible() override { return false; }
    std::unique_ptr<ContentProvider> getContentProvider() const override;
    void handle(Dirent* dirent, std::shared_ptr<Item> item) override;
    void handle(Dirent* dirent, const Hints& hints) override;

  protected:
    Dirent* createDirent() const override;

  private:
    std::unique_ptr<XapianIndexer> mp_indexer;
    CreatorData* mp_creatorData;
};

class TitleXapianHandler : public DirentHandler {
  public:
    explicit TitleXapianHandler(CreatorData* data);
    virtual ~TitleXapianHandler();

    void start() override;
    void stop() override;
    bool isCompressible() override { return false; }
    std::unique_ptr<ContentProvider> getContentProvider() const override;
    void handle(Dirent* dirent, std::shared_ptr<Item> item) override;
    void handle(Dirent* dirent, const Hints& hints) override;

  protected:
    Dirent* createDirent() const override;

  private: // function
    void handle_dirent(Dirent* dirent);

  private: // data
    std::unique_ptr<XapianIndexer> mp_indexer;
    CreatorData* mp_creatorData;
};

}
}

#endif // OPENZIM_LIBZIM_XAPIAN_WORKER_H
