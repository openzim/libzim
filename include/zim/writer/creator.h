/*
 * Copyright (C) 2017-2021 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2020, 2026 Veloman Yunkan
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

#ifndef ZIM_WRITER_CREATOR_H
#define ZIM_WRITER_CREATOR_H

#include <memory>
#include <zim/zim.h>
#include <zim/illustration.h>
#include <zim/writer/item.h>

namespace zim
{
  class Fileheader;
  namespace writer
  {
    class CreatorData;

    /**
     * A ZIM file should be created via `Creator` as follows:
     *
     * 1. Instantiate the `Creator` object.
     * 2. Configure it via the `config*()` methods as needed.
     * 3. Start ZIM creation with `startZimCreation()`.
     * 4. Add data to the ZIM file using the `add*()` methods.
     * 5. Set special data records in the ZIM file with the `set*()` methods
     *    (these are optional and may precede the data addition step).
     * 6. Finish ZIM creation with `finishZimCreation()`.
     *
     * All `add*()` methods and `finishZimCreation()` can throw an exception
     * (most of the time - though not necessarily - a subclass of
     * zim::CreatorError). It is up to the user to catch this exception and
     * handle the error.
     *
     * The current (documented) conditions when an exception is thrown are:
     *
     * - When an entry cannot be added (mainly because an entry with the same
     *   path has already been added) a `zim::InvalidEntry` will be thrown. The
     *   creator will still be in a valid state and the creation can continue.
     *
     * - When there are problems indexing the title a `zim::TitleIndexingError`
     *   is thrown. The creator will remain in a valid state, however it is not
     *   clear whether continuing the creation will result in a perfectly valid
     *   ZIM archive.
     *
     * - An exception has been thrown in a worker thread. This exception will
     *   be caught and rethrown via a `zim::AsyncError`. The creator will
     *   be put in an invalid state and creation cannot continue.
     *
     * - The creator is in error state.  A `zim::CreatorStateError` will be
     *   thrown.
     *
     * - Any exception thrown by the user implementation itself. Note that if
     *   this exception is thrown in a worker thread it will be exposed via an
     *   AsyncError.
     *
     * - Any other exception thrown for unknown reason.
     *
     * By default, the creator status is not changed by the thrown exception and
     * creation should stop.
     */
    class LIBZIM_API Creator
    {
      public:
        /**
         * Creator constructor.
         */
        Creator();
        virtual ~Creator();

        /**
         * Configure the verbosity of the creator
         *
         * @param verbose whether the creator should print verbose information.
         * @return a reference to itself.
         */
        Creator& configVerbose(bool verbose);

        /**
         * Configure the compression algorithm to use.
         *
         * @param comptype the compression algorithm to use.
         * @return a reference to itself.
         */
        Creator& configCompression(Compression compression);

        /**
         * Set the size of the created clusters.
         *
         * The creator will try to create clusters with (uncompressed) size as
         * close as possible to `targetSize` without exceeding that limit.  If
         * not possible (the only such case being an item larger than
         * `targetSize`) a separate cluster will be allocated for that
         * oversized item.
         *
         * Be careful with this value!
         *
         * Bigger value means more content put together, so a better
         * compression ratio. But it means also that more decompression has to
         * be done when reading a single item from a cluster. If you don't know
         * what value to use, don't configure the cluster size and let libzim
         * use the default value.
         *
         * @param targetSize The target size of a cluster (in bytes).
         * @return a reference to itself.
         */
        Creator& configClusterSize(zim::size_type targetSize);

        /**
         * Configure the fulltext indexing feature.
         *
         * @param indexing True if we must fulltext index the content.
         * @param language Language to use for the indexation.
         * @return a reference to itself.
         */
        Creator& configIndexing(bool indexing, const std::string& language);

        /**
         * Set the number of thread to use for the internal worker.
         *
         * @param nbWorkers The number of workers to use.
         * @return a reference to itself.
         */
        Creator& configNbWorkers(unsigned nbWorkers);

        /**
         * Start ZIM file creation.
         *
         * The creator must have been configured before calling this method.
         *
         * @param filepath the path of the ZIM file to create.
         */
        void startZimCreation(const std::string& filepath);

        /**
         * Add an item to the archive.
         *
         * @param item The item to add.
         */
        void addItem(std::shared_ptr<Item> item);

        /**
         * Add a metadata to the archive.
         *
         * @param name the name of the metadata
         * @param content the content of the metadata
         * @param mimetype the mimetype of the metadata. Only used to detect if
         *        the metadata must be compressed or not.
         */
        void addMetadata(const std::string& name,
                         const std::string& content,
                         const std::string& mimetype = "text/plain;charset=utf-8");

        /**
         * Add a metadata to the archive using a contentProvider instead of a
         * plain string.
         *
         * @param name the name of the metadata.
         * @param provider the provider of the content of the metadata.
         * @param mimetype the mimetype of the metadata. Only used to detect if
         *        the metadata must be compressed.
         */
        void addMetadata(const std::string& name,
                         std::unique_ptr<ContentProvider> provider,
                         const std::string& mimetype = "text/plain;charset=utf-8");

        /**
         * Add illustration to the archive.
         *
         * @param ii parameters of the illustration.
         * @param content the content of the illustration (must be a png
         *        content)
         */
        void addIllustration(const IllustrationInfo& ii,
                             const std::string& content);

        /**
         * Add illustration to the archive.
         *
         * @param ii parameters of the illustration.
         * @param provider the provider of the content of the illustration
         *        (must be a png content)
         */
        void addIllustration(const IllustrationInfo& ii,
                             std::unique_ptr<ContentProvider> provider);

        /**
         * Add illustration to the archive.
         *
         * @param size the size (width and height) of the illustration.
         * @param content the content of the illustration (must be a png
         *        content)
         */
        void addIllustration(unsigned int size, const std::string& content);

        /**
         * Add illustration to the archive.
         *
         * @param size the size (width and height) of the illustration.
         * @param provider the provider of the content of the illustration
         *        (must be a png content)
         */
        void addIllustration(unsigned int size,
                             std::unique_ptr<ContentProvider> provider);

        /**
         * Add a redirection entry to the archive.
         *
         * A redirection is an entry pointing to another entry (it is an analog
         * of a symbolic link in a file system).
         *
         * `addRedirection()` allows creating a redirection to a
         * not-yet-existent target entry, subject to the condition that the
         * target entry is present when `finishZimCreation()` is called (see
         * its documentation for additional information on the handling of
         * invalid redirections).
         *
         * Hints (especially FRONT_ARTICLE) can be used to put the redirection
         * in the front articles list.
         * By default, redirections are not front article.
         *
         * @param path the path of the redirection entry.
         * @param title the title of the redirection entry.
         * @param targetpath the path of the target of the redirection.
         * @param hints hints associated with the redirection.
         */
        void addRedirection(
            const std::string& path,
            const std::string& title,
            const std::string& targetpath,
            const Hints& hints = Hints());


        /** Add an alias of an existing entry.
         *
         * An alias is an entry sharing data with another entry (it is an
         * analog of a hard link in a file system).
         *
         * `addAlias()` clones the existing entry found at `targetPath`, sets
         * its title to `title` and stores the result under `path`.
         *
         * The alias entry will have the same type (redirection or item) as the
         * source entry.
         *
         * If the source entry is an item, the new entry will be an item
         * sharing the same MIME type and data with the existing item. (This is
         * different from the creation of a [redirection](@ref addRedirection())
         * to `targetPath`). However, the alias entry is not counted in the
         * media type counter and it is not fulltext indexed (only title
         * indexed).
         *
         * Hints can be given to influence creator handling (front article,
         * ...) as it is done for redirections.
         *
         * @param path the path of the alias (new) entry.  @param title the
         * title of the alias (new) entry.  @param targetPath the path of the
         * aliased (existing) entry.  @param hints hints associated with the
         * alias.
         */
        void addAlias(
            const std::string& path,
            const std::string& title,
            const std::string& targetPath,
            const Hints& hints = Hints());

        /**
         * Finalize ZIM file creation.
         *
         * During this stage invalid redirections and/or redirection chains
         * are detected and removed. A redirection is invalid if its target
         * path is missing from the ZIM file. A redirection chain is invalid
         * if it leads to an invalid redirection or ends with a loop. For
         * each redirection removed a message is printed on standard output.
         */
        void finishZimCreation();

        /**
         * Set the path of the main page.
         *
         * @param mainPath The path of the main page.
         */
        void setMainPath(const std::string& mainPath) { m_mainPath = mainPath; }

        /**
         * Set the uuid of the the archive.
         *
         * @param uuid The uuid of the archive.
         */
        void setUuid(const zim::Uuid& uuid) { m_uuid = uuid; }

      private:
        std::unique_ptr<CreatorData> data;

        // configuration
        bool m_verbose = false;
        Compression m_compression = Compression::Zstd;
        bool m_withIndex = false;
        size_t m_clusterSize;
        std::string m_indexingLanguage;
        unsigned m_nbWorkers = 4;

        // zim data
        std::string m_mainPath;
        Uuid m_uuid = Uuid::generate();

        void fillHeader(Fileheader* header) const;
        void writeLastParts() const;
        void checkError();
    };
  }

}

#endif // ZIM_WRITER_CREATOR_H
