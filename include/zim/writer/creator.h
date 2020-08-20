/*
 * Copyright (C) 2009 Tommi Maekitalo
 * Copyright (C) 20017-2020 Matthieu Gautier <mgautier@kymeria.fr>
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
#include <zim/writer/item.h>

namespace zim
{
  class Fileheader;
  namespace writer
  {
    class CreatorData;

    /**
     * The `Creator` is responsible to create a zim file.
     *
     * Once the `Creator` is instantiated, it can be configured with the
     * `set*` methods.
     * Then the creation process must be started with `startZimCreation`.
     * Elements of the zim file can be added using the `add*` methods.
     * The final steps is to call `finishZimCreation`.
     *
     * The `Creator` must be inherited and child class must reimplement
     * `get*` methods to provide further information.
     * Thoses `get*` methods will be called at the end of the process so
     * custom creator can deduce the value to return while they are creating
     * the zim file.
     */
    class Creator
    {
      public:

        /**
         * Creator constructor.
         *
         * @param verbose If the creator print verbose information.
         * @param comptype The compression algorithm to use.
         */
        Creator(bool verbose = false, CompressionType comptype = zimcompLzma);
        virtual ~Creator();

        zim::size_type getMinChunkSize() const { return minChunkSize; }

        /**
         * Set the minimum size of the cluster.
         *
         * Creator will try to create cluster with this minimum size (uncompressed size).
         *
         * @param size The minimum size of a cluster.
         */
        void setMinChunkSize(zim::size_type size) { minChunkSize = size; }

        /**
         * Configure the fulltext indexing feature.
         *
         * @param indexing True if we must fulltext index the content.
         * @param language Language to use for the indexation.
         */
        void setIndexing(bool indexing, std::string language)
        { withIndex = indexing; indexingLanguage = language; }

        /**
         * Set the number of thread to use for the internal worker.
         *
         * @param nbWorkers The number of workers to use.
         */
        void setNbWorkerThreads(unsigned nbWorkers) { nbWorkerThreads = nbWorkers; }


        /**
         * Start the zim creation.
         *
         * The creator must have been configured before calling this method.
         *
         * @param fname The filename of the zim to create.
         */
        void startZimCreation(const std::string& fname);

        /**
         * Add a item to the archive.
         *
         * @param item The item to add.
         */
        void addItem(std::shared_ptr<Item> item);

        /**
         * Add a metadata to the archive.
         *
         * @param name the name of the metadata
         * @param content the content of the metadata
         * @param mimetype the mimetype of the metadata.
         *                 Only used to detect if the metadata must be compressed or not.
         */
        void addMetadata(const std::string& name, const std::string& content, const std::string& mimetype = "text/plain");

        /**
         * Add a metadata to the archive using a contentProvider instead of plain string.
         *
         * @param name the name of the metadata.
         * @param provider the provider of the content of the metadata.
         * @param mimetype the mimetype of the metadata.
         *                 Only used to detect if the metadata must be compressed.
         */
        void addMetadata(const std::string& name, std::unique_ptr<ContentProvider> provider, const std::string& mimetype);

        /**
         * Add a redirection to the archive.
         *
         * @param path the path of the redirection.
         * @param title the title of the redirection.
         * @param targetpath the path of the target of the redirection
         */
        void addRedirection(
            const std::string& path,
            const std::string& title,
            const std::string& targetpath);

        /**
         * Finalize the zim creation.
         */
        void finishZimCreation();

        /**
         * Return the path of the main page.
         *
         * Must be reimplemented.
         *
         * @return The path of the main page.
         */
        virtual std::string getMainPath() const { return ""; }

        /**
         * Return the path of the favicon.
         *
         * Must be reimplemented.
         *
         * @return The path of the favicon.
         */
       virtual std::string getFaviconPath() const { return ""; }
        /**
         * Return the uuid of the archive.
         *
         * Must be reimplemented.
         *
         * @return The uuid of the archive.
         */
       virtual zim::Uuid getUuid() const { return Uuid::generate(); }

      private:
        std::unique_ptr<CreatorData> data;
        bool verbose;
        const CompressionType compression;
        bool withIndex = false;
        size_t minChunkSize = 1024-64;
        std::string indexingLanguage;
        unsigned nbWorkerThreads = 4;

        void fillHeader(Fileheader* header) const;
        void write() const;
    };
  }

}

#endif // ZIM_WRITER_CREATOR_H
