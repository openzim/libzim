/*
 * Copyright (C) 2020-2021 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2021 Maneesh P M <manu.pm55@gmail.com>
 * Copyright (C) 2020 Veloman Yunkan
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

#ifndef ZIM_ARCHIVE_H
#define ZIM_ARCHIVE_H

#include "zim.h"
#include "entry.h"
#include "uuid.h"

#include <string>
#include <vector>
#include <memory>
#include <bitset>
#include <set>

namespace zim
{
  class FileImpl;

  enum class EntryOrder {
    pathOrder,
    titleOrder,
    efficientOrder
  };

  /** Get the maximum size of the cluster cache.
   *
   * @return The maximum memory size used the cluster cache.
   */
  size_t LIBZIM_API getClusterCacheMaxSize();

  /** Get the current size of the cluster cache.
   *
   * @return The current memory size used by the cluster cache.
   */
  size_t LIBZIM_API getClusterCacheCurrentSize();

  /** Set the size of the cluster cache.
   *
   * If the new size is lower than the number of currently stored clusters
   * some clusters will be dropped from cache to respect the new size.
   *
   * @param sizeInB The memory limit (in bytes) for the cluster cache.
   */
  void LIBZIM_API setClusterCacheMaxSize(size_t sizeInB);


  /**
   * The Archive class to access content in a zim file.
   *
   * The `Archive` is the main class to access content in a zim file.
   * `Archive` are lightweight object and can be copied easily.
   *
   * An `Archive` is read-only, and internal states (as caches) are protected
   * from race-condition. Therefore, all methods of `Archive` are threadsafe.
   *
   * Zim archives exist with two different namespace schemes: An old one and the new one.
   * The method `hasNewNamespaceScheme` permit to know which namespace is used by the archive.
   *
   * When using old namespace scheme:
   * - User entries may be stored in different namespaces (historically `A`, `I`, `J` or `-`).
   *   So path of the entries contains the namespace as a "top level directory": `A/foo.html`, `I/image.png`, ...
   * - All API taking or returning a path expect/will return a path with the namespace.
   *
   * When using new namespace scheme:
   * - User entries are always stored without namespace.
   *   (For information, they are stored in the same namespace `C`. Still consider there is no namespace as all API masks it)
   *   As there is no namespace, paths don't contain it: `foo.hmtl`, `image.png`, ...
   * - All API taking or returning a path expect/will return a path without namespace.
   *
   * This difference may seem complex to handle, but not so much.
   * As all paths returned by API is consistent with paths expected, you simply have to use the path as it is.
   * Forget about the namespace and if a path has it, simply consider it as a subdirectory.
   * The only place it could be problematic is when you already have a path stored somewhere (bookmark, ...)
   * using a scheme and use it on an archive with another scheme. For this case, the method `getEntryByPath`
   * has a compatibility layer trying to transform a path to the new scheme as a fallback if the entry is not found.
   *
   * All methods of archive may throw an `ZimFileFormatError` if the file is invalid.
   */
  class LIBZIM_API Archive
  {
    public:
      template<EntryOrder order> class EntryRange;
      template<EntryOrder order> class iterator;

      /** Archive constructor.
       *
       *  Construct an archive from a filename.
       *  The file is open readonly.
       *
       *  The filename is the "logical" path.
       *  So if you want to open a split zim file (foo.zimaa, foo.zimab, ...)
       *  you must pass the `foo.zim` path.
       *
       *  @param fname The filename to the file to open (utf8 encoded)
       */
      explicit Archive(const std::string& fname);

      /** Archive constructor.
       *
       *  Construct an archive from a filename.
       *  The file is open readonly.
       *
       *  The filename is the "logical" path.
       *  So if you want to open a split zim file (foo.zimaa, foo.zimab, ...)
       *  you must pass the `foo.zim` path.
       *
       *  @param fname The filename to the file to open (utf8 encoded)
       *  @param openConfig The open configuration to use.
       */
      Archive(const std::string& fname, OpenConfig openConfig);

#ifndef _WIN32
      /** Archive constructor.
       *
       *  Construct an archive from a file descriptor.
       *  Fd is used only at Archive creation.
       *  Ownership of the fd is not taken and it must be closed by caller.
       *
       *  Note: This function is not available under Windows.
       *
       *  @param fd The descriptor of a seekable file representing a ZIM archive
       */
      explicit Archive(int fd);

      /** Archive constructor.
       *
       *  Construct an archive from a file descriptor.
       *  Fd is used only at Archive creation.
       *  Ownership of the fd is not taken and it must be closed by caller.
       *
       *  Note: This function is not available under Windows.
       *
       *  @param fd The descriptor of a seekable file representing a ZIM archive
       *  @param openConfig The open configuration to use.
       */
      Archive(int fd, OpenConfig openConfig);

      /** Archive constructor.
       *
       *  Construct an archive from a descriptor of a file with an embedded ZIM
       *  archive inside.
       *  Fd is used only at Archive creation.
       *  Ownership of the fd is not taken and it must be closed by caller.
       *
       *  Note: This function is not available under Windows.
       *
       *  @param fd The descriptor of a seekable file with a continuous segment
       *  representing a complete ZIM archive.
       *  @param offset The offset of the ZIM archive relative to the beginning
       *  of the file (rather than the current position associated with fd).
       *  @param size The size of the ZIM archive.
       */
       Archive(int fd, offset_type offset, size_type size);

      /** Archive constructor.
       *
       *  Construct an archive from a descriptor of a file with an embedded ZIM
       *  archive inside.
       *  Fd is used only at Archive creation.
       *  Ownership of the fd is not taken and it must be closed by caller.
       *
       *  Note: This function is not available under Windows.
       *
       *  @param fd The descriptor of a seekable file with a continuous segment
       *  representing a complete ZIM archive.
       *  @param offset The offset of the ZIM archive relative to the beginning
       *  of the file (rather than the current position associated with fd).
       *  @param size The size of the ZIM archive.
       *  @param openConfig The open configuration to use.
       */
       Archive(int fd, offset_type offset, size_type size, OpenConfig openConfig);

      /** Archive constructor.
       *
       *  Construct an archive from a descriptor of a file with an embedded ZIM
       *  archive inside.
       *  Fd is used only at Archive creation.
       *  Ownership of the fd is not taken and it must be closed by caller.
       *
       *  Note: This function is not available under Windows.
       *
       *  @param fd A FdInput (tuple) containing the fd (int), offset (offset_type) and size (size_type)
       *            referencing a continuous segment representing a complete ZIM archive.
       */
      explicit Archive(FdInput fd);

      /** Archive constructor.
       *
       *  Construct an archive from a descriptor of a file with an embedded ZIM
       *  archive inside.
       *  Fd is used only at Archive creation.
       *  Ownership of the fd is not taken and it must be closed by caller.
       *
       *  Note: This function is not available under Windows.
       *
       *  @param fd A FdInput (tuple) containing the fd (int), offset (offset_type) and size (size_type)
       *            referencing a continuous segment representing a complete ZIM archive.
       *  @param openConfig The open configuration to use.
       */
      Archive(FdInput fd, OpenConfig openConfig);

      /** Archive constructor.
       *
       *  Construct an archive from several file descriptors.
       *  Each part may be embedded in a file.
       *  Fds are used only at Archive creation.
       *  Ownership of the fds is not taken and they must be closed by caller.
       *  Fds (int) can be the same between FdInput if the parts belong to the same file.
       *
       *  Note: This function is not available under Windows.
       *
       *  @param fds A vector of FdInput (tuple) containing the fd (int), offset (offset_type) and size (size_type)
       *             referencing a series of segments representing a complete ZIM archive.
       */
      explicit Archive(const std::vector<FdInput>& fds);

      /** Archive constructor.
       *
       *  Construct an archive from several file descriptors.
       *  Each part may be embedded in a file.
       *  Fds are used only at Archive creation.
       *  Ownership of the fds is not taken and they must be closed by caller.
       *  Fds (int) can be the same between FdInput if the parts belong to the same file.
       *
       *  Note: This function is not available under Windows.
       *
       *  @param fds A vector of FdInput (tuple) containing the fd (int), offset (offset_type) and size (size_type)
       *             referencing a series of segments representing a complete ZIM archive.
       *  @param openConfig The open configuration to use.
       */
      Archive(const std::vector<FdInput>& fds, OpenConfig openConfig);
#endif

      /** Return the filename of the zim file.
       *
       *  Return the filename as passed to the constructor
       *  (So foo.zim).
       *
       *  @return The logical filename of the archive.
       */
      const std::string& getFilename() const;

      /** Return the logical archive size.
       *
       *  Return the size of the full archive, not the size of the file on the fs.
       *  If the zim is split, return the sum of the size of the parts.
       *
       *  @return The logical size of the archive.
       */
      size_type getFilesize() const;

      /** Return the number of entries in the archive.
       *
       * Return the total number of entries in the archive, including
       * internal entries created by libzim itself, metadata, indexes, ...
       *
       *  @return the number of all entries in the archive.
       */
      entry_index_type getAllEntryCount() const;

      /** Return the number of user entries in the archive.
       *
       * If the notion of "user entries" doesn't exist in the zim archive,
       * returns `getAllEntryCount()`.
       *
       *  @return the number of user entries in the archive.
       */
      entry_index_type getEntryCount() const;

      /** Return the number of articles in the archive.
       *
       *  The definition of "article" depends of the zim archive.
       *  On recent archives, this correspond to all entries marked as "FRONT_ARTICLE"
       *  at creaton time.
       *  On old archives, this corresponds to all "text/html*" entries.
       *
       *  @return the number of articles in the archive.
       */
      entry_index_type getArticleCount() const;

      /** Return the number of media in the archive.
       *
       * This definition of "media" is based on the mimetype.
       *
       * @return the number of media in the archive.
       */
      entry_index_type getMediaCount() const;

      /** The uuid of the archive.
       *
       *  @return the uuid of the archive.
       */
      Uuid getUuid() const;

      /** Get a specific metadata content.
       *
       *  Get the content of a metadata stored in the archive.
       *
       *  @param name The name of the metadata.
       *  @return The content of the metadata.
       *  @exception EntryNotFound If the metadata is not in the arcthive.
       */
      std::string getMetadata(const std::string& name) const;

      /** Get a specific metadata item.
       *
       *  Get the item associated to a metadata stored in the archive.
       *
       *  @param name The name of the metadata.
       *  @return The item associated to the metadata.
       *  @exception EntryNotFound If the metadata in not in the archive.
       */
      Item getMetadataItem(const std::string& name) const;

      /** Get the list of metadata stored in the archive.
       *
       *  @return The list of metadata in the archive.
       */
      std::vector<std::string> getMetadataKeys() const;

      /** Get the illustration item of the archive.
       *
       *  Illustration is a icon for the archive that can be used in catalog and so to illustrate the archive.
       *
       *  @param size The size (width and height) of the illustration to get. Default to 48 (48x48px icon)
       *  @return The illustration item.
       *  @exception EntryNotFound If no illustration item can be found.
       */
      Item getIllustrationItem(unsigned int size=48) const;

      /** Return a list of available sizes (width) for the illustations in the archive.
       *
       * Illustration is an icon for the archive that can be used in catalog and elsewehere to illustrate the archive.
       * An Archive may contains several illustrations with different size.
       * This method allows to know which illustration are in the archive (by size: width)
       *
       * @return A set of size.
       */
      std::set<unsigned int> getIllustrationSizes() const;


      /** Get an entry using its "path" index.
       *
       *  Use the index of the entry to get the idx'th entry
       *  (entry being sorted by path).
       *
       *  @param idx The index of the entry.
       *  @return The Entry.
       *  @exception std::out_of_range If idx is greater than the number of entry.
       */
      Entry getEntryByPath(entry_index_type idx) const;

      /** Get an entry using a path.
       *
       *  Search an entry in the zim, using its path.
       *  On archive with new namespace scheme, path must not contain the namespace.
       *  On archive without new namespace scheme, path must contain the namespace.
       *  A compatibility layer exists to accept "old" path on new archive (and the opposite)
       *  to help using saved path (bookmark) on new archive.
       *  On new archive, we first search the path in `C` namespace, then try to remove the potential namespace in path
       *  and search again in `C` namespace with path "without namespace".
       *  On old archive, we first assume path contains a namespace and if not (or no entry found) search in
       *  namespaces `A`, `I`, `J` and `-`.
       *
       *  @param path The entry's path.
       *  @return The Entry.
       *  @exception EntryNotFound If no entry has the asked path.
       */
      Entry getEntryByPath(const std::string& path) const;

      /** Get an entry using its "title" index.
       *
       *  Use the index of the entry to get the idx'th entry
       *  (entry being sorted by title).
       *
       *  @param idx The index of the entry.
       *  @return The Entry.
       *  @exception std::out_of_range If idx is greater than the number of entry.
       */
      Entry getEntryByTitle(entry_index_type idx) const;

      /** Get an entry using a title.
       *
       *  Get an entry using its title.
       *
       *  @param title The entry's title.
       *  @return The Entry.
       *  @exception EntryNotFound If no entry has the asked title.
       */
      Entry getEntryByTitle(const std::string& title) const;

      /** Get an entry using its "cluster" index.
       *
       *  Use the index of the entry to get the idx'th entry
       *  The actual order of the entry is not really specified.
       *  It is infered from the internal way the entry are stored.
       *
       *  This method is probably not relevent and is provided for completeness.
       *  You should probably use a iterator using the `efficientOrder`.
       *
       *  @param idx The index of the entry.
       *  @return The Entry.
       *  @exception std::out_of_range If idx is greater than the number of entry.
       */
      Entry getEntryByClusterOrder(entry_index_type idx) const;

      /** Get the main entry of the archive.
       *
       *  @return The Main entry.
       *  @exception EntryNotFound If no main entry has been specified in the archive.
       */
      Entry getMainEntry() const;

      /** Get a random entry.
       *
       * The entry is picked randomly from the front artice list.
       *
       * @return A random entry.
       * @exception EntryNotFound If no valid random entry can be found.
       */
      Entry getRandomEntry() const;

      /** Check in an entry has path in the archive.
       *
       *  The path follows the same requirement than `getEntryByPath`.
       *
       *  @param path The entry's path.
       *  @return True if the path in the archive, false else.
       */
      bool hasEntryByPath(const std::string& path) const {
        try{
          getEntryByPath(path);
          return true;
        } catch(...) { return false; }
      }

      /** Check in an entry has title in the archive.
       *
       *  @param title The entry's title.
       *  @return True if the title in the archive, false else.
       */
      bool hasEntryByTitle(const std::string& title) const {
        try{
          getEntryByTitle(title);
          return true;
        } catch(...) { return false; }
      }

      /** Check if archive has a main entry
       *
       * @return True if the archive has a main entry.
       */
      bool hasMainEntry() const;

      /** Check if archive has a favicon entry
       *
       * @param size The size (width and height) of the illustration to check. Default to 48 (48x48px icon)
       * @return True if the archive has a corresponding illustration entry.
       *         (Always True if the archive has no illustration, but a favicon)
       */
      bool hasIllustration(unsigned int size=48) const;

      /** Check if the archive has a fulltext index.
       *
       * @return True if the archive has a fulltext index
       */
      bool hasFulltextIndex() const;

      /** Check if the archive has a title index.
       *
       * @return True if the archive has a title index
       */
      bool hasTitleIndex() const;


      /** Get a "iterable" by path order.
       *
       *  This method allow to iterate on all user entries using a path order.
       *  If the notion of "user entries" doesn't exists (for old zim archive),
       *  this iterate on all entries in the zim file.
       *
       *  ```
       *  for(auto& entry:archive.iterByPath()) {
       *     ...
       *  }
       *  ```
       *
       *  @return A range on all the entries, in path order.
       */
      EntryRange<EntryOrder::pathOrder> iterByPath() const;

      /** Get a "iterable" by title order.
       *
       *  This method allow to iterate on all articles using a title order.
       *  The definition of "article" depends of the zim archive.
       *  On recent archives, this correspond to all entries marked as "FRONT_ARTICLE"
       *  at creaton time.
       *  On old archives, this correspond to all entries in 'A' namespace.
       *  Few archives may have been created without namespace but also without specific
       *  article listing. In this case, this iterate on all user entries.
       *
       *  ```
       *  for(auto& entry:archive.iterByTitle()) {
       *     ...
       *  }
       *  ```
       *
       *  @return A range on all the entries, in title order.
       */
      EntryRange<EntryOrder::titleOrder> iterByTitle() const;

      /** Get a "iterable" by a efficient order.
       *
       *  This method allow to iterate on all user entries using a effictient order.
       *  If the notion of "user entries" doesn't exists (for old zim archive),
       *  this iterate on all entries in the zim file.
       *
       *  ```
       *  for(auto& entry:archive.iterEfficient()) {
       *     ...
       *  }
       *  ```
       *
       *  @return A range on all the entries, in efficitent order.
       */
      EntryRange<EntryOrder::efficientOrder> iterEfficient() const;

      /** Find a range of entries starting with path.
       *
       * When using new namespace scheme, path must not contain the namespace (`foo.html`).
       * When using old namespace scheme, path must contain the namespace (`A/foo.html`).
       * Contrary to `getEntryByPath`, there is no compatibility layer, path must follow the archive scheme.
       *
       * @param path The path prefix to search for.
       * @return A range starting from the first entry starting with path
       *         and ending past the last entry.
       *         If no entry starts with `path`, begin == end.
       */
      EntryRange<EntryOrder::pathOrder>  findByPath(std::string path) const;

      /** Find a range of entry starting with title.
       *
       * When using old namespace scheme, entry title is search in `A` namespace.
       *
       * @param title The title prefix to search for.
       * @return A range starting from the first entry starting with title
       *         and ending past the last entry.
       *         If no entry starts with `title`, begin == end.
       */
      EntryRange<EntryOrder::titleOrder> findByTitle(std::string title) const;

      /** hasChecksum.
       *
       * The checksum is not the checksum of the file.
       * It is an internal checksum stored in the zim file.
       *
       * @return True if the archive has a checksum.
       */
      bool hasChecksum() const;

      /** getChecksum.
       *
       * @return the checksum stored in the archive.
       *         If the archive has no checksum return an empty string.
       */
      std::string getChecksum() const;

      /** Check that the zim file is valid (in regard to its checksum).
       *
       *  If the zim file has no checksum return false.
       *
       *  @return True if the file is valid.
       */
      bool check() const;

      /** Check the integrity of the zim file.
       *
       * Run different type of checks to verify the zim file is valid
       * (in regard to the zim format).
       * This may be time consuming.
       *
       * @return True if the file is valid.
       */
      bool checkIntegrity(IntegrityCheck checkType);

      /** Check if the file is split in the filesystem.
       *
       *  @return True if the archive is split in different file (foo.zimaa, foo.zimbb).
       */
      bool isMultiPart() const;

      /** Get if the zim archive uses the new namespace scheme.
       *
       * Recent zim file use the new namespace scheme.
       *
       * On user perspective, it means that :
       * - On old namespace scheme :
       *  . All entries are accessible, either using `getEntryByPath` with a specific namespace
       *    or simply iterating over the entries (with `iter*` methods).
       *  . Entry's path has namespace included ("A/foo.html")
       * - On new namespace scheme :
       *  . Only the "user" entries are accessible with `getEntryByPath` and `iter*` methods.
       *    To access metadatas, use `getMetadata` method.
       *  . Entry's path do not contains namespace ("foo.html")
       */
      bool hasNewNamespaceScheme() const;

      /** Get a shared ptr on the FileImpl
       *
       *  @internal
       *  @return The shared_ptr
       */
      std::shared_ptr<FileImpl> getImpl() const { return m_impl; }

      /** Get the size of the dirent cache.
       *
       * @return The maximum number of dirents stored in  the cache.
       */
      size_t getDirentCacheMaxSize() const;

      /** Get the current size of the dirent cache.
       *
       * @return The number of dirents currently stored in  the cache.
       */
      size_t getDirentCacheCurrentSize() const;

      /** Set the size of the dirent cache.
       *
       * If the new size is lower than the number of currently stored dirents
       * some dirents will be dropped from cache to respect the new size.
       *
       * @param nbDirents The maximum number of dirents stored in the cache.
       */
      void setDirentCacheMaxSize(size_t nbDirents);

#ifdef ZIM_PRIVATE
      cluster_index_type getClusterCount() const;
      offset_type getClusterOffset(cluster_index_type idx) const;
      entry_index_type getMainEntryIndex() const;

      /** Get an entry using a path and a namespace.
       *
       * @param ns The namespace to search in
       * @param path The entry's path (without namespace)
       * @return The entry
       * @exception EntryNotFound If no entry has been found.
       */
      Entry getEntryByPathWithNamespace(char ns, const std::string& path) const;
#endif

    private:
      std::shared_ptr<FileImpl> m_impl;
  };

  template<EntryOrder order>
  LIBZIM_API entry_index_type _toPathOrder(const FileImpl& file, entry_index_type idx);

  template<>
  LIBZIM_API entry_index_type _toPathOrder<EntryOrder::pathOrder>(const FileImpl& file, entry_index_type idx);
  template<>
  LIBZIM_API entry_index_type _toPathOrder<EntryOrder::titleOrder>(const FileImpl& file, entry_index_type idx);
  template<>
  LIBZIM_API entry_index_type _toPathOrder<EntryOrder::efficientOrder>(const FileImpl& file, entry_index_type idx);


  /**
   * A range of entries in an `Archive`.
   *
   * `EntryRange` represents a range of entries in a specific order.
   *
   * An `EntryRange` can't be modified is consequently threadsafe.
   */
  template<EntryOrder order>
  class LIBZIM_API Archive::EntryRange {
    public:
      explicit EntryRange(const std::shared_ptr<FileImpl> file, entry_index_type begin, entry_index_type end)
        : m_file(file),
          m_begin(begin),
          m_end(end)
      {}

      iterator<order> begin() const
        { return iterator<order>(m_file, entry_index_type(m_begin)); }
      iterator<order> end() const
        { return iterator<order>(m_file, entry_index_type(m_end)); }
      int size() const
        { return m_end - m_begin; }

      EntryRange<order> offset(int start, int maxResults) const
      {
        auto begin = m_begin + start;
        if (begin > m_end) {
          begin = m_end;
        }
        auto end = m_end;
        if (begin + maxResults < end) {
          end = begin + maxResults;
        }
        return EntryRange<order>(m_file, begin, end);
      }

private:
      std::shared_ptr<FileImpl> m_file;
      entry_index_type m_begin;
      entry_index_type m_end;
  };

  /**
   * An iterator on an `Archive`.
   *
   * `Archive::iterator` stores an internal state which is not protected
   * from race-condition. It is not threadsafe.
   *
   * An `EntryRange` can't be modified and is consequently threadsafe.
   *
   * Be aware that the referenced/pointed Entry is generated and stored
   * in the iterator itself.
   * Once the iterator is destructed or incremented/decremented, you must NOT
   * use the Entry.
   */
  template<EntryOrder order>
  class LIBZIM_API Archive::iterator
  {
    public:
      /* SuggestionIterator is conceptually a bidirectional iterator.
       * But std *LegayBidirectionalIterator* is also a *LegacyForwardIterator* and
       * it would impose us that :
       * > Given a and b, dereferenceable iterators of type It:
       * >  If a and b compare equal (a == b is contextually convertible to true)
       * >  then either they are both non-dereferenceable or *a and *b are references bound to the same object.
       * and
       * > the LegacyForwardIterator requirements requires dereference to return a reference.
       * Which cannot be as we create the entry on demand.
       *
       * So we are stick with declaring ourselves at `input_iterator`.
       */
      using iterator_category = std::input_iterator_tag;
      using value_type = Entry;
      using pointer = Entry*;
      using reference = Entry&;

      explicit iterator(const std::shared_ptr<FileImpl> file, entry_index_type idx)
        : m_file(file),
          m_idx(idx),
          m_entry(nullptr)
      {}

      iterator(const iterator<order>& other)
        : m_file(other.m_file),
          m_idx(other.m_idx),
          m_entry(other.m_entry?new Entry(*other.m_entry):nullptr)
      {}

      bool operator== (const iterator<order>& it) const
        { return m_file == it.m_file && m_idx == it.m_idx; }
      bool operator!= (const iterator<order>& it) const
        { return !operator==(it); }

      iterator<order>& operator=(iterator<order>&& it) = default;

      iterator<order>& operator=(iterator<order>& it)
      {
        m_entry.reset();
        m_idx = it.m_idx;
        m_file = it.m_file;
        return *this;
      }

      iterator<order>& operator++()
      {
        ++m_idx;
        m_entry.reset();
        return *this;
      }

      iterator<order> operator++(int)
      {
        auto it = *this;
        operator++();
        return it;
      }

      iterator<order>& operator--()
      {
        --m_idx;
        m_entry.reset();
        return *this;
      }

      iterator<order> operator--(int)
      {
        auto it = *this;
        operator--();
        return it;
      }

      const Entry& operator*() const
      {
        if (!m_entry) {
          m_entry.reset(new Entry(m_file, _toPathOrder<order>(*m_file, m_idx)));
        }
        return *m_entry;
      }

      const Entry* operator->() const
      {
        operator*();
        return m_entry.get();
      }

    private:
      std::shared_ptr<FileImpl> m_file;
      entry_index_type m_idx;
      mutable std::unique_ptr<Entry> m_entry;
  };

  /**
   * The set of the integrity checks to be performed by `zim::validate()`.
   */
  typedef std::bitset<size_t(IntegrityCheck::COUNT)> IntegrityCheckList;

  /** Check the integrity of the zim file.
   *
   * Run the specified checks to verify the zim file is valid
   * (with regard to the zim format). Some checks can be quite slow.
   *
   * @param zimPath The path of the ZIM archive to be checked.
   * @param checksToRun The set of checks to perform.
   * @return False if any check fails, true otherwise.
   */
  bool LIBZIM_API validate(const std::string& zimPath, IntegrityCheckList checksToRun);
}

#endif // ZIM_ARCHIVE_H

