/*
 * Copyright (C) 2020 Matthieu Gautier <mgautier@kymeria.fr>
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

#include "_dirent.h"
#include <zim/zim.h>
#include <zim/error.h>
#include "buffer.h"
#include "endian_tools.h"
#include "log.h"
#include <algorithm>
#include <cstring>

log_define("zim.dirent")

namespace zim {

namespace writer {

char NsAsChar(NS ns) {
  switch(ns) {
    case NS::C: return 'C';
    case NS::M: return 'M';
    case NS::W: return 'W';
    case NS::X: return 'X';
  }
  throw std::runtime_error("Invalid namespace value.");
}

// Creator for a "classic" dirent
Dirent::Dirent(NS ns, const std::string& path, const std::string& title, uint16_t mimetype)
  : pathTitle(path, title),
    mimeType(mimetype),
    idx(0),
    _ns(static_cast<uint8_t>(ns)),
    removed(false),
    _isFrontArticle(false)
{
  getDirect().cluster = nullptr;
  getDirect().blobNumber = blob_index_t(0);
}

// Creator for a resolved "redirection" dirent
Dirent::Dirent(NS ns, const std::string& path, const std::string& title, Dirent* target)
  : pathTitle(path, title),
    mimeType(redirectMimeType),
    idx(0),
    _ns(static_cast<uint8_t>(ns)),
    removed(false),
    _isFrontArticle(false)
{
  redirect.targetDirent = target;
}

Dirent::Dirent(const std::string& path, const std::string& title, const Dirent& target)
  : pathTitle(path, title),
    mimeType(target.mimeType),
    idx(0),
    _ns(target._ns),
    removed(false),
    _isFrontArticle(false)
{
  if ( target.isRedirect() ) {
    this->redirect = target.redirect;
  } else {
    this->direct = target.direct;
  }
}

NS Dirent::getRedirectNs() const {
  return getRedirectTargetDirent()->getNamespace();
}

std::string Dirent::getRedirectPath() const {
  return getRedirectTargetDirent()->getPath();
}

entry_index_t Dirent::getRedirectIndex() const      {
  const auto targetDirent = getRedirectTargetDirent();
  if ( targetDirent->isRemoved() ) {
    std::ostringstream oss;
    oss << NsAsChar(getNamespace()) << "/" << getPath();
    throw CreatorError("Dangling redirect remains at " + oss.str());
  }
  return targetDirent->getIdx();
}

} // namespace writer

} // namespace zim
