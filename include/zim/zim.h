/*
 * Copyright (C) 2020-2021 Veloman Yunkan
 * Copyright (C) 2018-2020 Matthieu Gautier <mgautier@kymeria.fr>
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
#include <string>

#ifdef __GNUC__
#define DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED __declspec(deprecated)
#else
#praga message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED
#endif

#include <zim/zim_config.h>

#if defined(_MSC_VER) && defined(LIBZIM_EXPORT_DLL)
    #define LIBZIM_API __declspec(dllexport)
#else
    #define LIBZIM_API
#endif

namespace zim
{
  // An index of an entry (in a zim file)
  typedef uint32_t entry_index_type;

  // An index of an cluster (in a zim file)
  typedef uint32_t cluster_index_type;

  // An index of a blog (in a cluster)
  typedef uint32_t blob_index_type;

  // The size of something (entry, zim, cluster, blob, ...)
  typedef uint64_t size_type;

  // An offset.
  typedef uint64_t offset_type;

  /**
   * Configuration to pass to archive constructors.
   *
   * This struct contains options controlling the opening of a ZIM archive. For
   * now, it is only related to preloading of data but it may change in the
   * future.
   *
   * Archive may eagerly preload certain data to speed up future operations.
   * However, such preloading itself takes some time.
   *
   * OpenConfig allows the user to define which data should be preloaded when
   * opening the archive.
   */
  struct LIBZIM_API OpenConfig {
     /**
      * Default configuration.
      *
      * - Dirent ranges is activated.
      * - Xapian preloading is activated.
      */
     OpenConfig();

     /**
      * Configure xapian preloading.
      *
      * This method modifies the configuration and returns itself.
      */
     OpenConfig& preloadXapianDb(bool load) {
       m_preloadXapianDb = load;
       return *this;
     }

     /**
      * Configure xapian preloading.
      *
      * This method creates a new configuration with the new value.
      */
     OpenConfig preloadXapianDb(bool load) const {
       return OpenConfig(*this).preloadXapianDb(load);
     }

     /**
      * Configure direntRanges preloading.
      *
      * libzim will load `nbRanges + 1` dirents to create `nbRanges` dirent
      * ranges. This will be used to speed up dirent lookup. This is an extra
      * layer on top of classic dirent cache.
      *
      * This method modifies the configuration and returns itself.
      */
     OpenConfig& preloadDirentRanges(int nbRanges) {
       m_preloadDirentRanges = nbRanges;
       return *this;
     }

     /**
      * Configure direntRanges preloading.
      *
      * libzim will load `nbRanges + 1` dirents to create `nbRanges` dirent
      * ranges. This will be used to speed up dirent lookup. This is an extra
      * layer on top of classic dirent cache.
      *
      * This method creates a new configuration with the new value.
      */
     OpenConfig preloadDirentRanges(int nbRanges) const {
       return OpenConfig(*this).preloadDirentRanges(nbRanges);
     }

     bool m_preloadXapianDb;
     int  m_preloadDirentRanges;
  };

  struct FdInput {
    // An open file descriptor
    int fd;

    // The (absolute) offset of the data "pointed" by FdInput in fd.
    offset_type offset;

    // The size (length) of the data "pointed" by FdInput
    size_type size;

    FdInput(int fd, offset_type offset, size_type size):
      fd(fd), offset(offset), size(size) {}
  };

  enum class Compression
  {
    None = 1,

    // intermediate values correspond to compression
    // methods that are no longer supported

    Zstd = 5
  };

  static const char MimeHtmlTemplate[] = "text/x-zim-htmltemplate";

  /**
   * Various types of integrity checks performed by `zim::validate()`.
   */
  enum class IntegrityCheck
  {
    /**
     * Validates the checksum of the ZIM file.
     */
    CHECKSUM,

    /**
     * Checks that offsets in PathPtrList are valid.
     */
    DIRENT_PTRS,

    /**
     * Checks that dirents are properly sorted.
     */
    DIRENT_ORDER,

    /**
     * Checks that entries in the title index are valid and properly sorted.
     */
    TITLE_INDEX,

    /**
     * Checks that offsets in ClusterPtrList are valid.
     */
    CLUSTER_PTRS,

    /**
     * Checks that offsets inside each clusters are valid.
     */
    CLUSTERS_OFFSETS,

    /**
     * Checks that mime-type values in dirents are valid.
     */
    DIRENT_MIMETYPES,

    ////////////////////////////////////////////////////////////////////////////
    // End of integrity check types.
    // COUNT must be the last one and denotes the count of all checks
    ////////////////////////////////////////////////////////////////////////////

    /**
     * `COUNT` is not a valid integrity check type. It exists to tell the
     * number of all supported integrity checks.
     */
    COUNT
  };

  /**
   * Information needed to directly access to an item data, bypassing libzim library.
   *
   * Some items may have their data store uncompressed in the zim archive.
   * In such case, an user can read the item data directly by (re)opening the file and
   * seek at the right offset.
   */
  struct ItemDataDirectAccessInfo {

     /**
      * The filename to open.
      */
     std::string filename;

     /**
      * The offset to seek to before reading.
      */
     offset_type offset;

     explicit ItemDataDirectAccessInfo()
       : filename(),
         offset()
     {}

     ItemDataDirectAccessInfo(const std::string& filename, offset_type offset)
       : filename(filename),
         offset(offset)
     {}

     /**
      * Return if the ItemDataDirectAccessInfo is valid
      */
     bool isValid() const {
      return !filename.empty();
     }
  };
}

#endif // ZIM_ZIM_H

