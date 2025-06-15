/*
 * Copyright (C) 2025 Veloman Yunkan
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

#ifndef ZIM_ILLUSTRATION_H
#define ZIM_ILLUSTRATION_H

#include "zim/zim.h"

#include <map>

namespace zim
{

class LIBZIM_API Attributes : public std::map<std::string, std::string>
{
  typedef std::map<std::string, std::string> ImplType;

public:
  Attributes() {}
  Attributes(const ImplType& x) : ImplType(x) {}

  Attributes(const std::initializer_list<ImplType::value_type>& x)
    : ImplType(x)
  {}
};

/**
 * Information about illustrations stored as metadata in the ZIM archive
 */
struct LIBZIM_API IllustrationInfo
{
  uint32_t width;  // in CSS pixels
  uint32_t height; // in CSS pixels
  float    scale;  // devicePixelRatio value of the targeted display media
  Attributes extraAttributes; // additional attributes of the illustration
                              // (not used yet but introduced beforehand in
                              // order to avoid an ABI change later)

  std::string asMetadataItemName() const;
  static IllustrationInfo fromMetadataItemName(const std::string& s);
};

inline bool operator==(const IllustrationInfo& a, const IllustrationInfo& b) {
  return a.width  == b.width
      && a.height == b.height
      && a.scale  == b.scale
      && a.extraAttributes == b.extraAttributes;
}

} // namespace zim

#endif // ZIM_ILLUSTRATION_H
