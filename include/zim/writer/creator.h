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
     * `config*` methods.
     * Then the creation process must be started with `startZimCreation`.
     * Elements of the zim file can be added using the `add*` methods.
     * The final steps is to call `finishZimCreation`.
     *
     * During the creation of the zim file (and before the call to `finishZimCreation`),
     * some values must be set using the `set*` methods.
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
        Creator();
        virtual ~Creator();

        /**
         * Configure the verbosity of the creator
         *
         * @param verbose if the creator print verbose information.
         * @return a reference to itself.
         */
        Creator& configVerbose(bool verbose);

        /**
         * Configure the compression algorithm to use.
         *
         * @param comptype the compression algorithm to use.
         * @return a reference to itself.
         */
        Creator& configCompression(CompressionType comptype);

        /**
         * Set the minimum size of the cluster.
         *
         * Creator will try to create cluster with this minimum size
         * (uncompressed size).
         *
         * @param size The minimum size of a cluster.
         * @return a reference to itself.
         */
        Creator& configMinClusterSize(zim::size_type size);

        /**
         * Configure the fulltext indexing feature.
         *
         * @param indexing True if we must fulltext index the content.
         * @param language Language to use for the indexation.
         * @return a reference to itself.
         */
        Creator& configIndexing(bool indexing, std::string language);

        /**
         * Set the number of thread to use for the internal worker.
         *
         * @param nbWorkers The number of workers to use.
         * @return a reference to itself.
         */
        Creator& configNbWorkers(unsigned nbWorkers);

        /**
         * Start the zim creation.
         *
         * The creator must have been configured before calling this method.
         *
         * @param filepath the path of the zim file to create.
         */
        void startZimCreation(const std::string& filepath);

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
            const std::string& targetpath,
            const Hints& hints = Hints());

        /**
         * Finalize the zim creation.
         */
        void finishZimCreation();

        /**
         * Set the path of the main page.
         *
         * @param mainPath The path of the main page.
         */
        void setMainPath(const std::string& mainPath) { m_mainPath = mainPath; }

        /**
         * Set the path of the favicon.
         *
         * @param faviconPath The path of the favicon.
         */
        void setFaviconPath(const std::string& faviconPath) { m_faviconPath = faviconPath; }

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
        CompressionType m_compression = zimcompZstd;
        bool m_withIndex = false;
        size_t m_minClusterSize = 1024-64;
        std::string m_indexingLanguage;
        unsigned m_nbWorkers = 4;

        // zim data
        std::string m_mainPath;
        std::string m_faviconPath;
        Uuid m_uuid = Uuid::generate();

        void fillHeader(Fileheader* header) const;
        void write() const;
        void checkError();
    };
  }

}

#endif // ZIM_WRITER_CREATOR_H
