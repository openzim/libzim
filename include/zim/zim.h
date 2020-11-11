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

#ifndef ZIM_ZIM_H
#define ZIM_ZIM_H

#include <cstdint>

#ifdef __GNUC__
#define DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED __declspec(deprecated)
#else
#praga message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED
#endif


namespace zim
{
  // An index of an article (in a zim file)
  typedef uint32_t article_index_type;

  // An index of an cluster (in a zim file)
  typedef uint32_t cluster_index_type;

  // An index of a blog (in a cluster)
  typedef uint32_t blob_index_type;

  // The size of something (article, zim, cluster, blob, ...)
  typedef uint64_t size_type;

  // An offset.
  typedef uint64_t offset_type;

  enum CompressionType
  {
    zimcompDefault,
    zimcompNone,
    zimcompZip, // Not supported anymore in the libzim
    zimcompBzip2, // Not supported anymore in the libzim
    zimcompLzma,
    zimcompZstd
  };

  static const char MimeHtmlTemplate[] = "text/x-zim-htmltemplate";

  enum class IntegrityCheck
  {
    CHECKSUM,
    DIRENT_PTRS, // Checks that offsets in UrlPtrList are valid
    TITLE_INDEX, // Checks that entries in the title index are valid
    CLUSTER_PTRS, // Checks that offsets in ClusterPtrList are valid

    // This must be the last one and denotes the count of all checks
    COUNT
  };
}

#endif // ZIM_ZIM_H

