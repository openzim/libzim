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

#include <zim/file.h>
#include <zim/fileiterator.h>
#include <zim/fileimpl.h>
#include <cxxtools/log.h>
#include <limits>

log_define("zim.file")

namespace zim
{
  File::File(const std::string& fname)
    : impl(new FileImpl(fname.c_str()))
  { }

  File::File(FileImpl* impl_)
    : impl(impl_)
  { }

  std::string File::readData(offset_type off, size_type count)
  {
    log_debug("readData(" << off << ", " << count << ')');
    return impl->readData(off, count);
  }

  std::string File::uncompressData(const Dirent& dirent, const std::string& data)
  {
    return impl->uncompressData(dirent, data);
  }

  Article File::getArticle(char ns, const std::string& url, bool collate)
  {
    log_debug("getArticle('" << ns << "', \"" << url << "\", " << collate << ')');
    return impl->getArticle(ns, url, collate);
  }

  Article File::getArticle(char ns, const QUnicodeString& url, bool collate)
  {
    log_debug(getFilename() << ".getArticle('" << ns << "', \"" << url << "\", " << collate << ')');
    return impl->getArticle(ns, url, collate);
  }

  Article File::getArticle(size_type idx)
  {
    log_debug(getFilename() << ".getArticle(" << idx << ')');
    return impl->getArticle(idx);
  }

  Dirent File::getDirent(size_type idx)
  {
    log_debug("getDirent(" << idx << ')');
    return impl->getDirent(idx);
  }

  size_type File::getCountArticles() const
  {
    log_debug("getCountArticles()");
    return impl->getCountArticles();
  }

  std::string File::getNamespaces()
  {
    log_debug("getNamespaces()");
    return impl->getNamespaces();
  }

  bool File::hasNamespace(char ch)
  {
    size_type off = getNamespaceBeginOffset(ch);
    return off < getCountArticles() && getDirent(off).getNamespace() == ch;
  }

  size_type File::getNamespaceBeginOffset(char ch)
  {
    log_debug("getNamespaceBeginOffset(" << ch << ')');
    return impl->getNamespaceBeginOffset(ch);
  }

  size_type File::getNamespaceEndOffset(char ch)
  {
    log_debug("getNamespaceEndOffset(" << ch << ')');
    return impl->getNamespaceEndOffset(ch);
  }

  File::const_iterator File::begin()
  {
    return const_iterator(this);
  }

  File::const_iterator File::end()
  {
    return const_iterator();
  }

  File::const_iterator File::find(char ns, const std::string& url, bool collate)
  {
    return const_iterator(this, impl->findArticle(ns, QUnicodeString(url), collate).second);
  }

  File::const_iterator File::find(char ns, const QUnicodeString& url, bool collate)
  {
    return const_iterator(this, impl->findArticle(ns, url, collate).second);
  }

  File::const_iterator::const_iterator(File* file_)
    : file(file_),
      idx(0)
  {
    article.setIndex(std::numeric_limits<zim::size_type>::max());
  }

  File::const_iterator::const_iterator(File* file_, size_type idx_)
    : file(file_),
      idx(idx_)
  {
    if (idx_ < file->getCountArticles())
      article = file->getArticle(idx);
    else
    {
      file = 0;
      idx = 0;
    }
  }

  Article File::const_iterator::operator*() const
  {
    log_debug("dereference File::const_iterator; articleindex=" << article.getIndex() << " idx=" << idx);
    if (article.getIndex() != idx)
    {
      log_debug("fetch article with index " << idx);
      article = file->getArticle(idx);
    }
    return article;
  }

}
