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
#include "dirent_lookup.h"
#include "zim/zim.h"

#ifdef ENABLE_XAPIAN
#include "search_internal.h"
#include "xapian.h"
#endif

#include "zim_types.h"
#include <memory>
#define CHUNK_SIZE 1024
#include <zim/error.h>
#include <zim/tools.h>
#include "_dirent.h"
#include "file_compound.h"
#include "buffer_reader.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <fstream>
#include <numeric>
#include "config.h"
#include "log.h"
#include "md5.h"
#include "tools.h"
#include "fileheader.h"

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
makeFileReader(std::shared_ptr<const FileCompound> zimFile)
{
  if (zimFile->fail()) {
    return nullptr;
  } else if ( zimFile->is_multiPart() ) {
    return std::make_shared<MultiPartFileReader>(zimFile);
  } else {
    const auto& firstAndOnlyPart = zimFile->begin()->second;
    return std::make_shared<FileReader>(firstAndOnlyPart->shareable_fhandle(), firstAndOnlyPart->offset(), firstAndOnlyPart->size());
  }
}

// Consider a set of integer-numbered objects with their object-ids spanning a
// contiguous range [a, b).
// Each object is also labelled with an integer group id. The group-ids too
// form a contiguous (or dense enough) set.
// The Grouping class allows to re-arrange the stream of such objects fed
// to it in the object-id order, returning a table of object-ids in the group-id
// order (where the order of the objects within the same group is preserved).
//
template<class ObjectId, class GroupId>
class Grouping
{
public: // types
  typedef std::vector<ObjectId> GroupedObjectIds;

public: // functions
  explicit Grouping(ObjectId objectIdBegin, ObjectId objectIdEnd)
    : firstObjectId_(objectIdBegin)
    , minGroupId_(std::numeric_limits<GroupId>::max())
    , maxGroupId_(std::numeric_limits<GroupId>::min())
  {
    groupIds_.reserve(objectIdEnd - objectIdBegin);
  }

  // i'th call of add() is assumed to refer to the object
  // with id (firstObjectId_+i)
  void add(GroupId groupId)
  {
    groupIds_.push_back(groupId);
    minGroupId_ = std::min(minGroupId_, groupId);
    maxGroupId_ = std::max(maxGroupId_, groupId);
  }

  GroupedObjectIds getGroupedObjectIds()
  {
    GroupedObjectIds result;
    if ( !groupIds_.empty() ) {
      // nextObjectSeat[g - minGroupId_] tells where the next object
      // with group-id g must be placed (seated) in the result
      std::vector<size_t> nextObjectSeat = getGroupBoundaries();

      result.resize(groupIds_.size());
      for ( size_t i = 0; i < groupIds_.size(); ++i ) {
        const GroupId g = groupIds_[i];
        // This statement has an important side-effect  vv
        const auto pos = nextObjectSeat[g - minGroupId_]++;
        result[pos] = firstObjectId_ + i;
      }
      GroupIds().swap(groupIds_);
    }
    return result;
  }

private: // functions
  std::vector<size_t> getGroupBoundaries() const
  {
    std::vector<size_t> groupIdCounts(maxGroupId_ - minGroupId_ + 1, 0);
    for ( const auto groupId : groupIds_ ) {
      ++groupIdCounts[groupId - minGroupId_];
    }

    std::vector<size_t> groupBoundaries(1, 0);
    std::partial_sum(groupIdCounts.begin(), groupIdCounts.end(),
                     std::back_inserter(groupBoundaries)
    );
    return groupBoundaries;
  }

private: // types
  typedef std::vector<GroupId> GroupIds;

private: // data
  const ObjectId firstObjectId_;
  GroupIds groupIds_;
  GroupId minGroupId_;
  GroupId maxGroupId_;
};

} //unnamed namespace

  ClusterCache& getClusterCache() {
    static ClusterCache clusterCache(CLUSTER_CACHE_SIZE);
    return clusterCache;
  }

  //////////////////////////////////////////////////////////////////////
  // FileImpl
  //
  FileImpl::FileImpl(const std::string& fname, OpenConfig openConfig)
    : FileImpl(FileCompound::openSinglePieceOrSplitZimFile(fname), openConfig)
  {}

#ifndef _WIN32
  FileImpl::FileImpl(int fd, OpenConfig openConfig)
    : FileImpl(std::make_shared<FileCompound>(fd), openConfig)
  {}

  FileImpl::FileImpl(FdInput fd, OpenConfig openConfig)
    : FileImpl(std::make_shared<FileCompound>(fd), openConfig)
  {}

  FileImpl::FileImpl(const std::vector<FdInput>& fds, OpenConfig openConfig)
    : FileImpl(std::make_shared<FileCompound>(fds), openConfig)
  {}
#endif

  FileImpl::FileImpl(std::shared_ptr<FileCompound> _zimFile, OpenConfig openConfig)
    : zimFile(_zimFile),
      zimReader(makeFileReader(zimFile)),
      direntReader(new DirentReader(zimReader)),
      m_hasFrontArticlesIndex(true),
      m_startUserEntry(0),
      m_endUserEntry(0)
#ifdef ENABLE_XAPIAN
      ,m_xapianDbCreated(false)
#endif
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

    // This can happen for several reasons:
    // - Zim file is corrupted (corrupted header)
    // - Zim file is too small (ongoing download, truncated file...)
    // - Zim file is embedded at beginning of another file (and we try to open the file as a zim file)
    //   If open through a FdInput, size should be set in FdInput.
    if (header.hasChecksum() && (header.getChecksumPos() + 16) != size_type(zimReader->size())) {
      throw ZimFileFormatError("Zim file(s) is of bad size or corrupted.");
    }

    auto pathPtrReader = sectionSubReader(*zimReader,
                                          "Dirent pointer table",
                                          offset_t(header.getPathPtrPos()),
                                          zsize_t(sizeof(offset_type)*header.getArticleCount()));

    mp_pathDirentAccessor.reset(
        new DirectDirentAccessor(direntReader, std::move(pathPtrReader), entry_index_t(header.getArticleCount())));

    clusterOffsetReader = sectionSubReader(*zimReader,
                                           "Cluster pointer table",
                                           offset_t(header.getClusterPtrPos()),
                                           zsize_t(sizeof(offset_type)*header.getClusterCount()));

    quickCheckForCorruptFile();

    if (openConfig.m_preloadDirentRanges == 0) {
      m_direntLookup = std::make_unique<DirentLookup>(mp_pathDirentAccessor.get());
    } else {
      m_direntLookup = std::make_unique<FastDirentLookup>(mp_pathDirentAccessor.get(), openConfig.m_preloadDirentRanges);
    }

    if (header.useNewNamespaceScheme()) {
      const_cast<entry_index_t&>(m_startUserEntry) = m_direntLookup->getNamespaceRangeBegin('C');
      const_cast<entry_index_t&>(m_endUserEntry) = m_direntLookup->getNamespaceRangeEnd('C');
    } else {
      const_cast<entry_index_t&>(m_endUserEntry) = getCountArticles();
    }

    // The following code may load clusters and we want to remove them from the
    // cache if something goes wrong.
    try {
      auto result = m_direntLookup->find('X', "listing/titleOrdered/v1");
      if (result.first) {
        mp_titleDirentAccessor = getTitleAccessorV1(result.second);
      }

      if (!mp_titleDirentAccessor) {
        if (!header.hasTitleListingV0()) {
          throw ZimFileFormatError("Zim file doesn't contain a title ordered index");
        }
        offset_t titleOffset(header.getTitleIdxPos());
        zsize_t  titleSize(sizeof(entry_index_type)*header.getArticleCount());
        mp_titleDirentAccessor = getTitleAccessor(titleOffset, titleSize, "Title index table");
        const_cast<bool&>(m_hasFrontArticlesIndex) = false;
      }
      m_byTitleDirentLookup.reset(new ByTitleDirentLookup(mp_titleDirentAccessor.get()));

#ifdef ENABLE_XAPIAN
      if (openConfig.m_preloadXapianDb) {
        mp_xapianDb = loadXapianDb();
        m_xapianDbCreated.store(true, std::memory_order_release);
      }
#endif

      readMimeTypes();
    } catch (...) {
      dropCachedClusters();
      throw;
    }
  }

  FileImpl::~FileImpl() {
    dropCachedClusters();
  }

  void FileImpl::dropCachedClusters() const {
    getClusterCache().dropAll([this](const ClusterRef& key) {
        return std::get<0>(key) == this;
    });
  }


  std::unique_ptr<IndirectDirentAccessor> FileImpl::getTitleAccessorV1(const entry_index_t idx)
  {
    auto dirent = mp_pathDirentAccessor->getDirent(idx);
    auto cluster = getCluster(dirent->getClusterNumber());
    if (cluster->isCompressed()) {
      // This is a ZimFileFormatError.
      // Let's be tolerent and skip the entry
      return nullptr;
    }
    auto titleOffset = getClusterOffset(dirent->getClusterNumber()) + cluster->getBlobOffset(dirent->getBlobNumber());
    auto titleSize = cluster->getBlobSize(dirent->getBlobNumber());
    return getTitleAccessor(titleOffset, titleSize, "Title index v1");
  }

  std::unique_ptr<IndirectDirentAccessor> FileImpl::getTitleAccessor(const offset_t offset, const zsize_t size, const std::string& name)
  {
      auto titleIndexReader = sectionSubReader(*zimReader,
                                               name,
                                               offset,
                                               size);

      return std::unique_ptr<IndirectDirentAccessor>(
        new IndirectDirentAccessor(mp_pathDirentAccessor, std::move(titleIndexReader), title_index_t(size.v/sizeof(entry_index_type))));
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
  }

  offset_type FileImpl::getMimeListEndUpperLimit() const
  {
    offset_type result(header.getPathPtrPos());
    if (header.hasTitleListingV0()) {
      result = std::min(result, header.getTitleIdxPos());
    }
    result = std::min(result, header.getClusterPtrPos());
    if ( getCountArticles().v != 0 ) {
      // assuming that dirents are placed in the zim file in the same
      // order as the corresponding entries in the dirent pointer table
      result = std::min(result, mp_pathDirentAccessor->getOffset(entry_index_t(0)).v);

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
    // - The old way by putting the pathPtrPos just after the mimetype.
    // - The new way by putting the pathPtrPos at the end of the zim files.
    //   In this case, the cluster data are always at 1024 bytes offset and we
    //   know that mimetype list is before this.
    // 1024 seems to be a good maximum size for the mimetype list, even for the
    // "old" way.
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
  }

  FileImpl::FindxResult FileImpl::findx(char ns, const std::string& path) const
  {
    return m_direntLookup->find(ns, path);
  }

  FileImpl::FindxResult FileImpl::findx(const std::string& longPath) const
  {
    char ns;
    std::string path;
    try {
      std::tie(ns, path) = parseLongPath(longPath);
      return findx(ns, path);
    } catch (...) {}
    return { false, entry_index_t(0) };
  }

  FileImpl::FindxTitleResult FileImpl::findxByTitle(char ns, const std::string& title)
  {
    return m_byTitleDirentLookup->find(ns, title);
  }

  FileCompound::PartRange
  FileImpl::getFileParts(offset_t offset, zsize_t size) const
  {
    return zimFile->locate(offset, size);
  }

  std::shared_ptr<const Dirent> FileImpl::getDirent(entry_index_t idx) const
  {
    return mp_pathDirentAccessor->getDirent(idx);
  }

  FileImpl::FindxResult FileImpl::findxMetadata(const std::string& name) const {
    auto r = findx('M', name);
    if (!r.first) {
      return r;
    }
    auto dirent_idx = r.second;
    auto dirent = getDirent(dirent_idx);
    auto watchdog = 50U;
    while (dirent->isRedirect() && --watchdog) {
      dirent_idx = dirent->getRedirectIndex();
      dirent = getDirent(dirent_idx);
    }
    if (watchdog) {
      return {true, dirent_idx};
    }
    return {false, entry_index_t(0)};
  }

  std::shared_ptr<const Dirent> FileImpl::getDirentByTitle(title_index_t idx) const
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

  void FileImpl::prepareArticleListByCluster() const
  {
    const auto endIdx = getEndUserEntry().v;
    const auto startIdx = getStartUserEntry().v;
    Grouping<entry_index_type, cluster_index_type> g(startIdx, endIdx);
    for(auto i = startIdx; i < endIdx; i++)
    {
      // This is the offset of the dirent in the zimFile
      auto indexOffset = mp_pathDirentAccessor->getOffset(entry_index_t(i));
      // Get the mimeType of the dirent (offset 0) to know the type of the dirent
      uint16_t mimeType = zimReader->read_uint<uint16_t>(indexOffset);
      if (mimeType==Dirent::redirectMimeType || mimeType==Dirent::linktargetMimeType || mimeType == Dirent::deletedMimeType) {
        g.add(0);
      } else {
        // If it is a classic article, get the clusterNumber (at offset 8)
        auto clusterNumber = zimReader->read_uint<zim::cluster_index_type>(indexOffset+offset_t(8));
        g.add(clusterNumber);
      }
    }
    m_articleListByCluster = g.getGroupedObjectIds();
  }

  entry_index_t FileImpl::getIndexByClusterOrder(entry_index_t idx) const
  {
    // Not using std::call_once because it is buggy. See the comment
    // in FileImpl::direntLookup().
    if ( m_articleListByCluster.empty() ) {
      std::lock_guard<std::mutex> lock(m_articleListByClusterMutex);
      if ( m_articleListByCluster.empty() ) {
        prepareArticleListByCluster();
      }
    }
    if (idx.v >= m_articleListByCluster.size())
      throw std::out_of_range("entry index out of range");
    return entry_index_t(m_articleListByCluster[idx.v]);
  }

  ClusterHandle FileImpl::readCluster(cluster_index_t idx) const
  {
    offset_t clusterOffset(getClusterOffset(idx));
    log_debug("read cluster " << idx << " from offset " << clusterOffset);
    return Cluster::read(*zimReader, clusterOffset);
  }

  ClusterHandle FileImpl::getCluster(cluster_index_t idx) const
  {
    if (idx >= getCountClusters())
      throw ZimFileFormatError("cluster index out of range");

    auto cluster_index_type = idx.v;
    auto key = std::make_tuple(this, cluster_index_type);
    auto cluster = getClusterCache().getOrPut(key, [=](){ return readCluster(idx); });
#if ENV32BIT
    // There was a bug in the way we create the zim files using ZSTD compression.
    // We were using a too hight compression level and so a window of 128Mb.
    // So at decompression, zstd reserve a 128Mb buffer.
    // While this memory is not really used (thanks to lazy allocation of OS),
    // we are still consumming address space. On 32bits this start to be a rare
    // ressource when we reserved 128Mb at once.
    // So we drop the cluster from the cache to avoid future memory allocation error.
    if (cluster->getCompression() == Cluster::Compression::Zstd) {
      // ZSTD compression starts to be used on version 5.0 of zim format.
      // Recently after, we switch to 5.1 and itegrate the fix in zstd creation.
      // 5.0 is not a perfect way to detect faulty zim file (it will generate false
      // positives) but it should be enough.
      if (header.getMajorVersion() == 5 && header.getMinorVersion() == 0) {
        getClusterCache().drop(key);
      }
    }
#endif
    return cluster;
  }

  offset_t FileImpl::getClusterOffset(cluster_index_t idx) const
  {
    return readOffset(*clusterOffsetReader, idx.v);
  }

  offset_t FileImpl::getBlobOffset(cluster_index_t clusterIdx, blob_index_t blobIdx) const
  {
    auto cluster = getCluster(clusterIdx);
    if (cluster->isCompressed())
      return offset_t(0);
    return getClusterOffset(clusterIdx) + cluster->getBlobOffset(blobIdx);
  }

  entry_index_t FileImpl::getNamespaceBeginOffset(char ch) const
  {
    log_trace("getNamespaceBeginOffset(" << ch << ')');
    return m_direntLookup->getNamespaceRangeBegin(ch);
  }

  entry_index_t FileImpl::getNamespaceEndOffset(char ch) const
  {
    log_trace("getNamespaceEndOffset(" << ch << ')');
    return m_direntLookup->getNamespaceRangeEnd(ch);
  }

  const std::string& FileImpl::getMimeType(uint16_t idx) const
  {
    if (idx >= mimeTypes.size())
      throw ZimFileFormatError(Formatter() << "unknown mime type code " << idx);

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

    unsigned char ch[CHUNK_SIZE];
    offset_type checksumPos = header.getChecksumPos();
    offset_type toRead = checksumPos;

    for(auto part = zimFile->begin();
        part != zimFile->end();
        part++) {
      std::ifstream stream(part->second->filename(), std::ios_base::in|std::ios_base::binary);

      while(toRead>=CHUNK_SIZE && stream.read(reinterpret_cast<char*>(ch),CHUNK_SIZE).good()) {
        zim_MD5Update(&md5ctx, ch, CHUNK_SIZE);
        toRead-=CHUNK_SIZE;
      }

      // Previous read was good, so we have exited the previous `while` because
      // `toRead<CHUNK_SIZE`. Let's try to read `toRead` chars and process them later.
      // Else, the previous `while` exited because we didn't succeed to read
      // `CHUNK_SIZE`, and we still have some data to process before changing part.
      // It reads the remaining amount of part when we reach the end of the file
      if(stream.good()){
        stream.read(reinterpret_cast<char*>(ch),toRead);
      }

      // It updates the checksum with the remaining amount of data when we
      // reach the end of the file or part
      zim_MD5Update(&md5ctx, ch, stream.gcount());
      toRead-=stream.gcount();

      if (stream.bad()) {
        perror("error while reading file");
        return false;
      }
      if (!toRead) {
        break;
      }
    }

    if (toRead) {
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
      case IntegrityCheck::CLUSTERS_OFFSETS: return FileImpl::checkClusters();
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
      const auto offset = mp_pathDirentAccessor->getOffset(entry_index_t(i));
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
      const std::shared_ptr<const Dirent> dirent = mp_pathDirentAccessor->getDirent(entry_index_t(i));
      if ( prevDirent && !(prevDirent->getLongPath() < dirent->getLongPath()) )
      {
        std::cerr << "Dirent table is not properly sorted:\n"
                  << "  #" << i-1 << ": " << prevDirent->getLongPath() << "\n"
                  << "  #" << i   << ": " << dirent->getLongPath() << std::endl;
        return false;
      }
      prevDirent = dirent;
    }
    return true;
  }

  bool FileImpl::checkClusters() {
    const cluster_index_type clusterCount = getCountClusters().v;
    for ( cluster_index_type i = 0; i < clusterCount; ++i )
    {
      // Force a read of each clusters (which will throw ZimFileFormatError in case of error)
      try {
        readCluster(cluster_index_t(i));
      } catch (ZimFileFormatError& e) {
        std::cerr << e.what() << std::endl;
        return false;
      }
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

    auto ret = true;
    if (header.hasTitleListingV0()) {
      offset_t titleOffset(header.getTitleIdxPos());
      zsize_t  titleSize(sizeof(entry_index_type)*header.getArticleCount());
      auto titleDirentAccessor = getTitleAccessor(titleOffset, titleSize, "Full Title index table");
      ret = checkTitleListing(*titleDirentAccessor, articleCount);
    }

    auto titleDirentAccessor = std::unique_ptr<IndirectDirentAccessor>();
    auto result = m_direntLookup->find('X', "listing/titleOrdered/v1");
    if (result.first) {
      titleDirentAccessor = getTitleAccessorV1(result.second);
    }
    if (titleDirentAccessor) {
      ret &= checkTitleListing(*titleDirentAccessor, articleCount);
    }
    return ret;
  }

  bool FileImpl::checkDirentMimeTypes() {
    const entry_index_type articleCount = getCountArticles().v;
    for ( entry_index_type i = 0; i < articleCount; ++i )
    {
      const auto dirent = mp_pathDirentAccessor->getDirent(entry_index_t(i));
      if ( dirent->isArticle() && dirent->getMimeType() >= mimeTypes.size() ) {
        std::cerr << "Entry " << dirent->getLongPath()
                  << " has invalid MIME-type value " << dirent->getMimeType()
                  << "." << std::endl;
        return false;
      }
    }
    return true;
  }

  size_t FileImpl::getDirentCacheMaxSize() const {
    return mp_pathDirentAccessor->getMaxCacheSize();
  }
  size_t FileImpl::getDirentCacheCurrentSize() const {
    return mp_pathDirentAccessor->getCurrentCacheSize();
  }
  void FileImpl::setDirentCacheMaxSize(size_t nbDirents) {
    mp_pathDirentAccessor->setMaxCacheSize(nbDirents);
  }

  ItemDataDirectAccessInfo FileImpl::getDirectAccessInformation(cluster_index_t clusterIdx, blob_index_t blobIdx) const
  {
    auto cluster = getCluster(clusterIdx);
    if (cluster->isCompressed()) {
      return ItemDataDirectAccessInfo();
    }

    auto full_offset = getBlobOffset(clusterIdx, blobIdx);

    auto part_its = getFileParts(full_offset, zsize_t(cluster->getBlobSize(blobIdx)));
    auto first_part = part_its.first;
    if (++part_its.first != part_its.second) {
     // The content is split on two parts. We cannot have direct access
      return ItemDataDirectAccessInfo();
    }
    auto range = first_part->first;
    auto part = first_part->second;
    const offset_type logical_local_offset(full_offset - range.min);
    const auto physical_local_offset = logical_local_offset + part->offset().v;
    return ItemDataDirectAccessInfo(part->filename(), physical_local_offset);
  }

  Blob FileImpl::getBlob(const Dirent& dirent, offset_t offset) const
  {
    auto cluster = getCluster(dirent.getClusterNumber());
    auto blobIdx = dirent.getBlobNumber();
    auto size = zsize_t(cluster->getBlobSize(blobIdx).v - offset.v);
    return cluster->getBlob(blobIdx, offset, size);
  }

  Blob FileImpl::getBlob(const Dirent& dirent, offset_t offset, zsize_t size) const
  {
    auto cluster = getCluster(dirent.getClusterNumber());
    return cluster->getBlob(dirent.getBlobNumber(), offset, size);
  }

#ifdef ENABLE_XAPIAN
  std::shared_ptr<XapianDb> FileImpl::loadXapianDb() {
    FileImpl::FindxResult r;
    r = m_direntLookup->find('X', "fulltext/xapian");
    if (!r.first) {
      r = m_direntLookup->find('Z', "/fulltextIndex/xapian");
    }
    if (!r.first) {
      return nullptr;
    }
    auto xapianDirent = getDirent(r.second);
    if (xapianDirent->isRedirect()) {
      return nullptr;
    }
    auto accessInfo = getDirectAccessInformation(xapianDirent->getClusterNumber(), xapianDirent->getBlobNumber());
    if (!accessInfo.isValid()) {
      return nullptr;
    }

    Xapian::Database xapianDatabase;
    if (!getDbFromAccessInfo(accessInfo, xapianDatabase)) {
      return nullptr;
    }

    try {
      std::string defaultLanguage;
      // Database created before 2017/03 has no language metadata.
      // However, term were stemmed anyway and we need to stem our
      // search query the same the database was created.
      // So we need a language, let's use the one of the zim.
      // If zimfile has no language metadata, we can't do lot more here :/
      try {
        auto r = findxMetadata("Language");
        if (r.first) {
          auto langDirent = getDirent(r.second);
          defaultLanguage = getBlob(*langDirent);
        }
      } catch(...) {}

      return std::make_shared<XapianDb>(xapianDatabase, defaultLanguage);
    } catch (Xapian::DatabaseError& e) {
      // Do nothing
    }
    return nullptr;
  }

  std::shared_ptr<XapianDb> FileImpl::getXapianDb() {
    if (!m_xapianDbCreated.load(std::memory_order_acquire)) {
      std::lock_guard<std::mutex> lock(m_xapianDbCreationMutex);
      if (!m_xapianDbCreated.load(std::memory_order_acquire)) {
        loadXapianDb();
        m_xapianDbCreated.store(true, std::memory_order_release);
      }
    }
    return mp_xapianDb;
  }
#endif
}
