/*
 * Copyright (C) 2017-2021 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2020-2021 Veloman Yunkan
 * Copyright (C) 2006,2009 Tommi Maekitalo
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

#include "fileimpl.h"
#include <zim/error.h>
#include "_dirent.h"
#include "file_compound.h"
#include "buffer_reader.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <errno.h>
#include <cstring>
#include <fstream>
#include "config.h"
#include "log.h"
#include "envvalue.h"
#include "md5.h"
#include "tools.h"

log_define("zim.file.impl")

namespace zim
{

namespace
{

offset_t readOffset(const Reader& reader, entry_index_type idx)
{
  offset_t offset(reader.read_uint<offset_type>(offset_t(sizeof(offset_type)*idx)));
  return offset;
}

std::unique_ptr<const Reader>
sectionSubReader(const Reader& zimReader, const std::string& sectionName,
                 offset_t offset, zsize_t size)
{
  if (!zimReader.can_read(offset, size)) {
    throw ZimFileFormatError(sectionName + " outside (or not fully inside) ZIM file.");
  }
#ifdef ENABLE_USE_BUFFER_HEADER
  const auto buf = zimReader.get_buffer(offset, size);
  return std::unique_ptr<Reader>(new BufferReader(buf));
#else
  return zimReader.sub_reader(offset, size);
#endif
}

std::shared_ptr<Reader>
makeFileReader(std::shared_ptr<const FileCompound> zimFile, offset_t offset, zsize_t size)
{
  if (zimFile->fail()) {
    return nullptr;
  } else if ( zimFile->is_multiPart() ) {
    ASSERT(offset.v, ==, 0u);
    ASSERT(size, ==, zimFile->fsize());
    return std::make_shared<MultiPartFileReader>(zimFile);
  } else {
    const auto& firstAndOnlyPart = zimFile->begin()->second;
    return std::make_shared<FileReader>(firstAndOnlyPart->shareable_fhandle(), offset, size);
  }
}

} //unnamed namespace

  //////////////////////////////////////////////////////////////////////
  // FileImpl
  //
  FileImpl::FileImpl(const std::string& fname)
    : FileImpl(std::make_shared<FileCompound>(fname))
  {}

#ifndef _WIN32
  FileImpl::FileImpl(int fd)
    : FileImpl(std::make_shared<FileCompound>(fd))
  {}

  FileImpl::FileImpl(int fd, offset_t offset, zsize_t size)
    : FileImpl(std::make_shared<FileCompound>(fd), offset, size)
  {}
#endif

  FileImpl::FileImpl(std::shared_ptr<FileCompound> _zimFile)
    : FileImpl(_zimFile, offset_t(0), _zimFile->fsize())
  {}

  FileImpl::FileImpl(std::shared_ptr<FileCompound> _zimFile, offset_t offset, zsize_t size)
    : zimFile(_zimFile),
      archiveStartOffset(offset),
      zimReader(makeFileReader(zimFile, offset, size)),
      direntReader(new DirentReader(zimReader)),
      clusterCache(envValue("ZIM_CLUSTERCACHE", CLUSTER_CACHE_SIZE)),
      m_newNamespaceScheme(false),
      m_hasFrontArticlesIndex(true),
      m_startUserEntry(0),
      m_endUserEntry(0)
  {
    log_trace("read file \"" << zimFile->filename() << '"');

    if (zimFile->fail())
      throw ZimFileFormatError(std::string("can't open zim-file \"") + zimFile->filename() + '"');

    // read header
    if (size_type(zimReader->size()) < Fileheader::size) {
      throw ZimFileFormatError("zim-file is too small to contain a header");
    }
    try {
      header.read(*zimReader);
    } catch (ZimFileFormatError& e) {
      throw e;
    } catch (...) {
      throw ZimFileFormatError("error reading zim-file header.");
    }

    auto urlPtrReader = sectionSubReader(*zimReader,
                                         "Dirent pointer table",
                                         offset_t(header.getUrlPtrPos()),
                                         zsize_t(sizeof(offset_type)*header.getArticleCount()));

    mp_urlDirentAccessor.reset(
        new DirectDirentAccessor(direntReader, std::move(urlPtrReader), entry_index_t(header.getArticleCount())));


    clusterOffsetReader = sectionSubReader(*zimReader,
                                           "Cluster pointer table",
                                           offset_t(header.getClusterPtrPos()),
                                           zsize_t(sizeof(offset_type)*header.getClusterCount()));

    quickCheckForCorruptFile();

    mp_titleDirentAccessor = getTitleAccessor("listing/titleOrdered/v1");

    if (!mp_titleDirentAccessor) {
      offset_t titleOffset(header.getTitleIdxPos());
      zsize_t  titleSize(sizeof(entry_index_type)*header.getArticleCount());
      mp_titleDirentAccessor = getTitleAccessor(titleOffset, titleSize, "Title index table");
      const_cast<bool&>(m_hasFrontArticlesIndex) = false;
    }
    m_byTitleDirentLookup.reset(new ByTitleDirentLookup(mp_titleDirentAccessor.get()));

    readMimeTypes();
  }

  std::unique_ptr<IndirectDirentAccessor> FileImpl::getTitleAccessor(const std::string& path)
  {
    auto result = direntLookup().find('X', path);
    if (!result.first) {
      return nullptr;
    }

    auto dirent = mp_urlDirentAccessor->getDirent(result.second);
    auto cluster = getCluster(dirent->getClusterNumber());
    if (cluster->isCompressed()) {
      // This is a ZimFileFormatError.
      // Let's be tolerent and skip the entry
      return nullptr;
    }
    auto titleOffset = getClusterOffset(dirent->getClusterNumber()) + cluster->getBlobOffset(dirent->getBlobNumber());
    auto titleSize = cluster->getBlobSize(dirent->getBlobNumber());
    return getTitleAccessor(titleOffset, titleSize, "Title index table" + path);
  }

  std::unique_ptr<IndirectDirentAccessor> FileImpl::getTitleAccessor(const offset_t offset, const zsize_t size, const std::string& name)
  {
      auto titleIndexReader = sectionSubReader(*zimReader,
                                               name,
                                               offset,
                                               size);

      return std::unique_ptr<IndirectDirentAccessor>(
        new IndirectDirentAccessor(mp_urlDirentAccessor, std::move(titleIndexReader), title_index_t(size.v/sizeof(entry_index_type))));
  }

  FileImpl::DirentLookup& FileImpl::direntLookup() const
  {
    std::call_once(m_direntLookupOnceFlag, [this]{

      const auto cacheSize = envValue("ZIM_DIRENTLOOKUPCACHE", DIRENT_LOOKUP_CACHE_SIZE);
      m_direntLookup.reset(new DirentLookup(mp_urlDirentAccessor.get(), cacheSize));
    });
    return *m_direntLookup;
  }

  void FileImpl::quickCheckForCorruptFile()
  {
    if (!getCountClusters())
      log_warn("no clusters found");
    else
    {
      offset_t lastOffset = getClusterOffset(cluster_index_t(cluster_index_type(getCountClusters()) - 1));
      log_debug("last offset=" << lastOffset.v << " file size=" << getFilesize().v);
      if (lastOffset.v > getFilesize().v)
      {
        log_fatal("last offset (" << lastOffset << ") larger than file size (" << getFilesize() << ')');
        throw ZimFileFormatError("last cluster offset larger than file size; file corrupt");
      }
    }

    if (header.hasChecksum() && header.getChecksumPos() != (getFilesize().v-16) ) {
      throw ZimFileFormatError("Checksum position is not valid");
    }
  }

  offset_type FileImpl::getMimeListEndUpperLimit() const
  {
    offset_type result(header.getUrlPtrPos());
    result = std::min(result, header.getTitleIdxPos());
    result = std::min(result, header.getClusterPtrPos());
    if ( getCountArticles().v != 0 ) {
      // assuming that dirents are placed in the zim file in the same
      // order as the corresponding entries in the dirent pointer table
      result = std::min(result, mp_urlDirentAccessor->getOffset(entry_index_t(0)).v);

      // assuming that clusters are placed in the zim file in the same
      // order as the corresponding entries in the cluster pointer table
      result = std::min(result, readOffset(*clusterOffsetReader, 0).v);
    }
    return result;
  }

  void FileImpl::readMimeTypes()
  {
    // read mime types
    // libzim write zims files two ways :
    // - The old way by putting the urlPtrPos just after the mimetype.
    // - The new way by putting the urlPtrPos at the end of the zim files.
    //   In this case, the cluster data are always at 1024 bytes offset and we know that
    //   mimetype list is before this.
    // 1024 seems to be a good maximum size for the mimetype list, even for the "old" way.
    const auto endMimeList = getMimeListEndUpperLimit();
    if ( endMimeList <= header.getMimeListPos() ) {
        throw(ZimFileFormatError("Bad ZIM archive"));
    }
    const zsize_t size(endMimeList - header.getMimeListPos());
    if ( endMimeList > 1024 ) {
        log_warn("The MIME-type list is abnormally large (" << size.v << " bytes)");
    }
    auto buffer = zimReader->get_buffer(offset_t(header.getMimeListPos()), size);
    const char* const bufferEnd = buffer.data() + size.v;
    const char* p = buffer.data();
    while (*p != '\0') {
      const char* zp = std::find(p, bufferEnd, '\0');

      if (zp == bufferEnd) {
        throw(ZimFileFormatError("Error getting mimelists."));
      }

      std::string mimeType(p, zp);
      mimeTypes.push_back(mimeType);

      p = zp+1;
    }

    const_cast<bool&>(m_newNamespaceScheme) = header.getMinorVersion() >= 1;
    if (m_newNamespaceScheme) {
      const_cast<entry_index_t&>(m_startUserEntry) = getNamespaceBeginOffset('C');
      const_cast<entry_index_t&>(m_endUserEntry) = getNamespaceEndOffset('C');
    } else {
      const_cast<entry_index_t&>(m_endUserEntry) = getCountArticles();
    }
  }

  FileImpl::FindxResult FileImpl::findx(char ns, const std::string& url)
  {
    return direntLookup().find(ns, url);
  }

  FileImpl::FindxResult FileImpl::findx(const std::string& url)
  {
    char ns;
    std::string path;
    try {
      std::tie(ns, path) = parseLongPath(url);
      return findx(ns, path);
    } catch (...) {}
    return { false, entry_index_t(0) };
  }

  static inline int direntCompareTitle(char ns, const std::string& title, const Dirent& dirent)
  {
    auto direntNs = dirent.getNamespace();
    if (ns < direntNs) {
      return -1;
    }
    if (ns > direntNs) {
      return 1;
    }
    return title.compare(dirent.getTitle());
  }

  FileImpl::FindxTitleResult FileImpl::findxByTitle(char ns, const std::string& title)
  {
    return m_byTitleDirentLookup->find(ns, title);
  }

  FileCompound::PartRange
  FileImpl::getFileParts(offset_t offset, zsize_t size)
  {
    return zimFile->locate(offset, size);
  }

  std::shared_ptr<const Dirent> FileImpl::getDirent(entry_index_t idx)
  {
    return mp_urlDirentAccessor->getDirent(idx);
  }

  std::shared_ptr<const Dirent> FileImpl::getDirentByTitle(title_index_t idx)
  {
    return mp_titleDirentAccessor->getDirent(idx);
  }

  entry_index_t FileImpl::getIndexByTitle(title_index_t idx) const
  {
    return mp_titleDirentAccessor->getDirectIndex(idx);
  }

  entry_index_t FileImpl::getFrontEntryCount() const
  {
    return entry_index_t(mp_titleDirentAccessor->getDirentCount().v);
  }

  entry_index_t FileImpl::getIndexByClusterOrder(entry_index_t idx) const
  {
      std::call_once(orderOnceFlag, [this]
      {
          articleListByCluster.reserve(getUserEntryCount().v);

          auto endIdx = getEndUserEntry().v;
          for(auto i = getStartUserEntry().v; i < endIdx; i++)
          {
              // This is the offset of the dirent in the zimFile
              auto indexOffset = mp_urlDirentAccessor->getOffset(entry_index_t(i));
              // Get the mimeType of the dirent (offset 0) to know the type of the dirent
              uint16_t mimeType = zimReader->read_uint<uint16_t>(indexOffset);
              if (mimeType==Dirent::redirectMimeType || mimeType==Dirent::linktargetMimeType || mimeType == Dirent::deletedMimeType) {
                articleListByCluster.push_back(std::make_pair(0, i));
              } else {
                // If it is a classic article, get the clusterNumber (at offset 8)
                auto clusterNumber = zimReader->read_uint<zim::cluster_index_type>(indexOffset+offset_t(8));
                articleListByCluster.push_back(std::make_pair(clusterNumber, i));
              }
          }
          std::sort(articleListByCluster.begin(), articleListByCluster.end());
      });

      if (idx.v >= articleListByCluster.size())
        throw std::out_of_range("entry index out of range");
      return entry_index_t(articleListByCluster[idx.v].second);
  }

  FileImpl::ClusterHandle FileImpl::readCluster(cluster_index_t idx)
  {
    offset_t clusterOffset(getClusterOffset(idx));
    log_debug("read cluster " << idx << " from offset " << clusterOffset);
    return Cluster::read(*zimReader, clusterOffset);
  }

  std::shared_ptr<const Cluster> FileImpl::getCluster(cluster_index_t idx)
  {
    if (idx >= getCountClusters())
      throw ZimFileFormatError("cluster index out of range");

    auto cluster = clusterCache.getOrPut(idx.v, [=](){ return readCluster(idx); });
#if ENV32BIT
    // There was a bug in the way we create the zim files using ZSTD compression.
    // We were using a too hight compression level and so a window of 128Mb.
    // So at decompression, zstd reserve a 128Mb buffer.
    // While this memory is not really used (thanks to lazy allocation of OS),
    // we are still consumming address space. On 32bits this start to be a rare
    // ressource when we reserved 128Mb at once.
    // So we drop the cluster from the cache to avoid future memory allocation error.
    if (cluster->getCompression() == Compression::Zstd) {
      // ZSTD compression starts to be used on version 5.0 of zim format.
      // Recently after, we switch to 5.1 and itegrate the fix in zstd creation.
      // 5.0 is not a perfect way to detect faulty zim file (it will generate false
      // positives) but it should be enough.
      if (header.getMajorVersion() == 5 && header.getMinorVersion() == 0) {
        clusterCache.drop(idx.v);
      }
    }
#endif
    return cluster;
  }

  offset_t FileImpl::getClusterOffset(cluster_index_t idx) const
  {
    return readOffset(*clusterOffsetReader, idx.v);
  }

  offset_t FileImpl::getBlobOffset(cluster_index_t clusterIdx, blob_index_t blobIdx)
  {
    auto cluster = getCluster(clusterIdx);
    if (cluster->isCompressed())
      return offset_t(0);
    return getClusterOffset(clusterIdx) + cluster->getBlobOffset(blobIdx);
  }

  entry_index_t FileImpl::getNamespaceBeginOffset(char ch) const
  {
    log_trace("getNamespaceBeginOffset(" << ch << ')');
    return direntLookup().getNamespaceRangeBegin(ch);
  }

  entry_index_t FileImpl::getNamespaceEndOffset(char ch) const
  {
    log_trace("getNamespaceEndOffset(" << ch << ')');
    return direntLookup().getNamespaceRangeEnd(ch);
  }

  const std::string& FileImpl::getMimeType(uint16_t idx) const
  {
    if (idx >= mimeTypes.size())
    {
      std::ostringstream msg;
      msg << "unknown mime type code " << idx;
      throw ZimFileFormatError(msg.str());
    }

    return mimeTypes[idx];
  }

  std::string FileImpl::getChecksum()
  {
    if (!header.hasChecksum())
      return std::string();

    try {
      auto chksum = zimReader->get_buffer(offset_t(header.getChecksumPos()), zsize_t(16));

      char hexdigest[33];
      hexdigest[32] = '\0';
      static const char hex[] = "0123456789abcdef";
      char* p = hexdigest;
      for (int i = 0; i < 16; ++i)
      {
        uint8_t v = chksum.at(offset_t(i));
        *p++ = hex[v >> 4];
        *p++ = hex[v & 0xf];
      }
      log_debug("chksum=" << hexdigest);
      return hexdigest;
    } catch (...)
    {
      log_warn("error reading checksum");
      return std::string();
    }
  }

  bool FileImpl::verify()
  {
    if (!header.hasChecksum())
      return false;

    struct zim_MD5_CTX md5ctx;
    zim_MD5Init(&md5ctx);

    offset_type checksumPos = header.getChecksumPos();
    offset_type currentPos = 0;
    for(auto part = zimFile->begin();
        part != zimFile->end();
        part++) {
      std::ifstream stream(part->second->filename(), std::ios_base::in|std::ios_base::binary);

      char ch;
      for(/*NOTHING*/ ; currentPos < checksumPos && stream.get(ch).good(); currentPos++) {
        zim_MD5Update(&md5ctx, reinterpret_cast<const uint8_t*>(&ch), 1);
      }
      if (stream.bad()) {
        perror("error while reading file");
        return false;
      }
      if (currentPos == checksumPos) {
        break;
      }
    }

    if (currentPos != checksumPos) {
      return false;
    }

    unsigned char chksumCalc[16];
    auto chksumFile = zimReader->get_buffer(offset_t(header.getChecksumPos()), zsize_t(16));

    zim_MD5Final(chksumCalc, &md5ctx);
    if (std::memcmp(chksumFile.data(), chksumCalc, 16) != 0)
    {
      return false;
    }

    return true;
  }

  time_t FileImpl::getMTime() const {
    return zimFile->getMTime();
  }

  zim::zsize_t FileImpl::getFilesize() const {
    return zimReader->size();
  }

  bool FileImpl::is_multiPart() const {
    return zimFile->is_multiPart();
  }

  bool FileImpl::checkIntegrity(IntegrityCheck checkType) {
    switch(checkType) {
      case IntegrityCheck::CHECKSUM: return FileImpl::checkChecksum();
      case IntegrityCheck::DIRENT_PTRS: return FileImpl::checkDirentPtrs();
      case IntegrityCheck::DIRENT_ORDER: return FileImpl::checkDirentOrder();
      case IntegrityCheck::TITLE_INDEX: return FileImpl::checkTitleIndex();
      case IntegrityCheck::CLUSTER_PTRS: return FileImpl::checkClusterPtrs();
      case IntegrityCheck::DIRENT_MIMETYPES: return FileImpl::checkDirentMimeTypes();
      case IntegrityCheck::COUNT: ASSERT("shouldn't have reached here", ==, "");
    }
    return false;
  }

  bool FileImpl::checkChecksum() {
    if ( ! verify() ) {
        std::cerr << "Checksum doesn't match" << std::endl;
        return false;
    }
    return true;
  }

  bool FileImpl::checkDirentPtrs() {
    const entry_index_type articleCount = getCountArticles().v;
    const offset_t validDirentRangeStart(80); // XXX: really???
    const offset_t validDirentRangeEnd = header.hasChecksum()
                                       ? offset_t(header.getChecksumPos())
                                       : offset_t(zimReader->size().v);
    const zsize_t direntMinSize(11);
    for ( entry_index_type i = 0; i < articleCount; ++i )
    {
      const auto offset = mp_urlDirentAccessor->getOffset(entry_index_t(i));
      if ( offset < validDirentRangeStart ||
           offset + direntMinSize > validDirentRangeEnd ) {
        std::cerr << "Invalid dirent pointer" << std::endl;
        return false;
      }
    }
    return true;
  }

  bool FileImpl::checkDirentOrder() {
    const entry_index_type articleCount = getCountArticles().v;
    std::shared_ptr<const Dirent> prevDirent;
    for ( entry_index_type i = 0; i < articleCount; ++i )
    {
      const std::shared_ptr<const Dirent> dirent = mp_urlDirentAccessor->getDirent(entry_index_t(i));
      if ( prevDirent && !(prevDirent->getLongUrl() < dirent->getLongUrl()) )
      {
        std::cerr << "Dirent table is not properly sorted:\n"
                  << "  #" << i-1 << ": " << prevDirent->getLongUrl() << "\n"
                  << "  #" << i   << ": " << dirent->getLongUrl() << std::endl;
        return false;
      }
      prevDirent = dirent;
    }
    return true;
  }

  bool FileImpl::checkClusterPtrs() {
    const cluster_index_type clusterCount = getCountClusters().v;
    const offset_t validClusterRangeStart(80); // XXX: really???
    const offset_t validClusterRangeEnd = header.hasChecksum()
                                       ? offset_t(header.getChecksumPos())
                                       : offset_t(zimReader->size().v);
    const zsize_t clusterMinSize(1); // XXX
    for ( cluster_index_type i = 0; i < clusterCount; ++i )
    {
      const auto offset = readOffset(*clusterOffsetReader, i);
      if ( offset < validClusterRangeStart ||
           offset + clusterMinSize > validClusterRangeEnd ) {
        std::cerr << "Invalid cluster pointer" << std::endl;
        return false;
      }
    }
    return true;
  }

namespace
{

std::string pseudoTitle(const Dirent& d)
{
  return std::string(1, d.getNamespace()) + '/' + d.getTitle();
}

bool checkTitleListing(const IndirectDirentAccessor& accessor, entry_index_type totalCount) {
  const entry_index_type direntCount = accessor.getDirentCount().v;
  std::shared_ptr<const Dirent> prevDirent;
  for ( entry_index_type i = 0; i < direntCount; ++i ) {
    if (accessor.getDirectIndex(title_index_t(i)).v >= totalCount) {
      std::cerr << "Invalid title index entry." << std::endl;
      return false;
    }

    const std::shared_ptr<const Dirent> dirent = accessor.getDirent(title_index_t(i));
    if ( prevDirent && !(pseudoTitle(*prevDirent) <= pseudoTitle(*dirent)) ) {
      std::cerr << "Title index is not properly sorted." << std::endl;
      return false;
    }
    prevDirent = dirent;
  }
  return true;
}

} // unnamed namespace

  bool FileImpl::checkTitleIndex() {
    const entry_index_type articleCount = getCountArticles().v;

    offset_t titleOffset(header.getTitleIdxPos());
    zsize_t  titleSize(sizeof(entry_index_type)*header.getArticleCount());
    auto titleDirentAccessor = getTitleAccessor(titleOffset, titleSize, "Full Title index table");
    auto ret = checkTitleListing(*titleDirentAccessor, articleCount);

    titleDirentAccessor = getTitleAccessor("listing/titleOrdered/v1");
    if (titleDirentAccessor) {
      ret &= checkTitleListing(*titleDirentAccessor, articleCount);
    }
    return ret;
  }

  bool FileImpl::checkDirentMimeTypes() {
    const entry_index_type articleCount = getCountArticles().v;
    for ( entry_index_type i = 0; i < articleCount; ++i )
    {
      const auto dirent = mp_urlDirentAccessor->getDirent(entry_index_t(i));
      if ( dirent->isArticle() && dirent->getMimeType() >= mimeTypes.size() ) {
        std::cerr << "Entry " << dirent->getLongUrl()
                  << " has invalid MIME-type value " << dirent->getMimeType()
                  << "." << std::endl;
        return false;
      }
    }
    return true;
  }

}
