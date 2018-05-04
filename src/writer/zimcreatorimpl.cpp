/*
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

#include "zimcreatorimpl.h"
#include "cluster.h"
#include <zim/blob.h>
#include <zim/writer/zimcreator.h>
#include "../endian_tools.h"
#include <algorithm>
#include <fstream>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <limits>
#include <stdexcept>
#include <sstream>
#include "config.h"
#include "md5stream.h"
#include "tee.h"
#include "log.h"

log_define("zim.writer.creator")

#define INFO(e) \
    do { \
        log_info(e); \
        std::cout << e << std::endl; \
    } while(false)

namespace zim
{
  namespace writer
  {
    ZimCreatorImpl::ZimCreatorImpl(bool verbose)
      : minChunkSize(1024-64),
        nextMimeIdx(0),
        compression(zimcompLzma),
        verbose(verbose)
    {
    }

    ZimCreatorImpl::~ZimCreatorImpl()
    {
      if (compCluster)
        delete compCluster;
      if (uncompCluster)
        delete uncompCluster;
    }

    void ZimCreatorImpl::create(const std::string& fname, ArticleSource& src)
    {
      isEmpty = true;
      isExtended = false;

      std::string basename = fname;
      basename =  (fname.size() > 4 && fname.compare(fname.size() - 4, 4, ".zim") == 0)
                     ? fname.substr(0, fname.size() - 4)
                     : fname;
      log_debug("basename " << basename);
      src.setFilename(fname);

      INFO("create directory entries");
      createDirentsAndClusters(src, basename + ".tmp");
      INFO(dirents.size() << " directory entries created");

      INFO("create title index");
      createTitleIndex(src);
      INFO(dirents.size() << " title index created");
      INFO(clusterOffsets.size() << " clusters created");

      INFO("fill header");
      fillHeader(src);

      INFO("write zimfile");
      write(basename + ".zim", basename + ".tmp");

      ::remove((basename + ".tmp").c_str());

      INFO("ready");
    }

    Dirent ZimCreatorImpl::createDirentFromArticle(const Article* article)
    {
      Dirent dirent;
      dirent.setAid(article->getAid());
      dirent.setUrl(article->getNamespace(), article->getUrl());
      dirent.setTitle(article->getTitle());
      dirent.setParameter(article->getParameter());

      log_debug("article " << dirent.getLongUrl() << " fetched");

      if (article->isRedirect())
      {
        dirent.setRedirect(article_index_t(0));
        dirent.setRedirectAid(article->getRedirectAid());
        log_debug("is redirect to " << dirent.getRedirectAid());
      }
      else if (article->isLinktarget())
      {
        dirent.setLinktarget();
      }
      else if (article->isDeleted())
      {
        dirent.setDeleted();
      }
      else
      {
        dirent.setArticle(getMimeTypeIdx(article->getMimeType()), cluster_index_t(0), blob_index_t(0));
        dirent.setCompress(article->shouldCompress());
        log_debug("is article; mimetype " << dirent.getMimeType());
      }
      return dirent;
    }


    void ZimCreatorImpl::closeCluster(bool compressed)
    {
      Cluster *cluster;
      DirentPtrsType *myDirents, *otherDirents;
      nbClusters++;
      if (compressed )
      {
        cluster = compCluster;
        myDirents = &compDirents;
        otherDirents = &uncompDirents;
        nbCompClusters++;
      } else {
        cluster = uncompCluster;
        myDirents = &uncompDirents;
        otherDirents = &compDirents;
        nbUnCompClusters++;
      }
      clusterOffsets.push_back(offset_t(tmp_out.tellp()));
      tmp_out << *cluster;
      log_debug("cluster written");
      if (cluster->is_extended() )
        isExtended = true;
      cluster->clear();
      myDirents->clear();
      // Update the cluster number of the dirents *not* written to disk.
      for (DirentPtrsType::iterator dpi = otherDirents->begin();
           dpi != otherDirents->end(); ++dpi)
      {
        Dirent *di = &dirents[*dpi];
        di->setCluster(cluster_index_t(clusterOffsets.size()), di->getBlobNumber());
      }
    }

    void ZimCreatorImpl::addDirent(const Dirent& dirent, const Article* article)
    {
      dirents.push_back(dirent);

      // If this is a redirect, we're done: there's no blob to add.
      if (dirent.isRedirect())
      {
        return;
      }

      // Add blob data to compressed or uncompressed cluster.
      auto articleSize = article->getSize();
      if (articleSize > 0)
      {
        isEmpty = false;
      }

      Cluster *cluster;
      DirentPtrsType *myDirents;
      if (dirent.isCompress())
      {
        cluster = compCluster;
        myDirents = &compDirents;
      }
      else
      {
        cluster = uncompCluster;
        myDirents = &uncompDirents;
      }

      // If cluster will be too large, write it to dis, and open a new
      // one for the content.
      if ( cluster->count()
        && cluster->size().v+articleSize >= minChunkSize * 1024
         )
      {
        log_info("cluster with " << cluster->count() << " articles, " <<
                 cluster->size() << " bytes; current title \"" <<
                 dirent.getTitle() << '\"');
        closeCluster(dirent.isCompress());
      }

      dirents.back().setCluster(cluster_index_t(clusterOffsets.size()), cluster->count());
      cluster->addArticle(article);
      myDirents->push_back(dirents.size()-1);
    }

    void ZimCreatorImpl::createDirentsAndClusters(ArticleSource& src, const std::string& tmpfname)
    {
      INFO("collect articles");
      tmp_out = std::ofstream(tmpfname.c_str());

      // We keep both a "compressed cluster" and an "uncompressed cluster"
      // because we don't know which one will fill up first.  We also need
      // to track the dirents currently in each, so we can fix up the
      // cluster index if the other one ends up written first.
      compCluster = new Cluster(compression);
      uncompCluster = new Cluster(zimcompNone);

      const Article* article;
      while ((article = src.getNextArticle()) != 0)
      {
        Dirent dirent = createDirentFromArticle(article);
        addDirent(dirent, article);
        nbArticles++;
        if (dirent.isCompress())
          nbCompArticles++;
        else
          nbUnCompArticles++;
        if (!article->getFilename().empty())
          nbFileArticles++;
        if (verbose && nbArticles%1000 == 0){
          std::cout << "A:" << nbArticles
                    << "; CA:" << nbCompArticles
                    << "; UA:" << nbUnCompArticles
                    << "; FA:" << nbFileArticles
                    << "; C:" << nbClusters
                    << "; CC:" << nbCompClusters
                    << "; UC:" << nbUnCompClusters
                    << std::endl;
        }
      }
      if (verbose) {
        std::cout << "A:" << nbArticles
                  << "; CA:" << nbCompArticles
                  << "; UA:" << nbUnCompArticles
                  << "; FA:" << nbFileArticles
                  << "; C:" << nbClusters
                  << "; CC:" << nbCompClusters
                  << "; UC:" << nbUnCompClusters
                  << std::endl;
      }

      // When we've seen all articles, write any remaining clusters.
      if (compCluster->count())
        closeCluster(true);

      if (uncompCluster->count())
        closeCluster(false);

      if (!tmp_out)
      {
        throw std::runtime_error("failed to write temporary cluster file");
      }

      clustersSize = zsize_t(tmp_out.tellp());

      // sort
      INFO("sort " << dirents.size() << " directory entries (aid)");
      std::sort(dirents.begin(), dirents.end(), compareAid);

      // remove invalid redirects
      INFO("remove invalid redirects from " << dirents.size() << " directory entries");
      DirentsType::size_type di = 0;
      while (di < dirents.size())
      {
        if (di % 10000 == 0)
          INFO(di << "directory entries checked for invalid redirects");

        if (dirents[di].isRedirect())
        {
          log_debug("check " << dirents[di].getTitle() << " redirect to " << dirents[di].getRedirectAid() << " (" << di << '/' << dirents.size() << ')');

          if (!std::binary_search(dirents.begin(), dirents.end(), Dirent(dirents[di].getRedirectAid()), compareAid))
          {
            log_debug("remove invalid redirection " << dirents[di].getTitle());
            dirents.erase(dirents.begin() + di);
            continue;
          }
        }

        ++di;
      }

      // sort
      INFO("sort " << dirents.size() << " directory entries (url)");
      std::sort(dirents.begin(), dirents.end(), compareUrl);

      // set index
      INFO("set index");
      article_index_t idx(0);
      for (DirentsType::iterator di = dirents.begin(); di != dirents.end(); ++di) {
        di->setIdx(idx);
        idx += 1;
      }

      // sort
      log_debug("sort " << dirents.size() << " directory entries (aid)");
      std::sort(dirents.begin(), dirents.end(), compareAid);

      // translate redirect aid to index
      INFO("translate redirect aid to index");
      for (DirentsType::iterator di = dirents.begin(); di != dirents.end(); ++di)
      {
        if (di->isRedirect())
        {
          DirentsType::iterator ddi = std::lower_bound(dirents.begin(), dirents.end(), di->getRedirectAid(), compareAid);
          if (ddi != dirents.end() && ddi->getAid() == di->getRedirectAid())
          {
            log_debug("redirect aid=" << ddi->getAid() << " redirect index=" << ddi->getIdx());
            di->setRedirect(ddi->getIdx());
          }
          else
          {
            std::ostringstream msg;
            msg << "internal error: redirect aid " << di->getRedirectAid() << " not found";
            log_fatal(msg.str());
            throw std::runtime_error(msg.str());
          }
        }
      }

      // sort
      log_debug("sort " << dirents.size() << " directory entries (url)");
      std::sort(dirents.begin(), dirents.end(), compareUrl);
      tmp_out.close();
    }

    namespace
    {
      class CompareTitle
      {
          ZimCreatorImpl::DirentsType& dirents;

        public:
          explicit CompareTitle(ZimCreatorImpl::DirentsType& dirents_)
            : dirents(dirents_)
            { }
          bool operator() (article_index_t titleIdx1, article_index_t titleIdx2) const
          {
            Dirent d1 = dirents[article_index_type(titleIdx1)];
            Dirent d2 = dirents[article_index_type(titleIdx2)];
            return d1.getNamespace() < d2.getNamespace()
               || (d1.getNamespace() == d2.getNamespace()
                && d1.getTitle() < d2.getTitle());
          }
      };
    }

    void ZimCreatorImpl::createTitleIndex(ArticleSource& src)
    {
      titleIdx.resize(dirents.size());
      for (DirentsType::size_type n = 0; n < dirents.size(); ++n)
        titleIdx[n] = dirents[n].getIdx();

      CompareTitle compareTitle(dirents);
      std::sort(titleIdx.begin(), titleIdx.end(), compareTitle);
    }

    void ZimCreatorImpl::fillHeader(ArticleSource& src)
    {
      std::string mainAid = src.getMainPage();
      std::string layoutAid = src.getLayoutPage();

      log_debug("main aid=" << mainAid << " layout aid=" << layoutAid);

      if (isExtended) {
        header.setMajorVersion(Fileheader::zimExtendedMajorVersion);
      } else {
        header.setMajorVersion(Fileheader::zimClassicMajorVersion);
      }
      header.setMinorVersion(Fileheader::zimMinorVersion);
      header.setMainPage(std::numeric_limits<article_index_type>::max());
      header.setLayoutPage(std::numeric_limits<article_index_type>::max());

      if (!mainAid.empty() || !layoutAid.empty())
      {
        for (DirentsType::const_iterator di = dirents.begin(); di != dirents.end(); ++di)
        {
          if (mainAid == di->getAid())
          {
            log_debug("main idx=" << di->getIdx());
            header.setMainPage(article_index_type(di->getIdx()));
          }

          if (layoutAid == di->getAid())
          {
            log_debug("layout idx=" << di->getIdx());
            header.setLayoutPage(article_index_type(di->getIdx()));
          }
        }
      }

      header.setUuid( src.getUuid() );
      header.setArticleCount( dirents.size() );
      header.setUrlPtrPos( offset_type(urlPtrPos()));
      header.setMimeListPos( offset_type(mimeListPos()) );
      header.setTitleIdxPos( offset_type(titleIdxPos()) );
      header.setClusterCount( clusterOffsets.size() );
      header.setClusterPtrPos( offset_type(clusterPtrPos()) );
      header.setChecksumPos( offset_type(checksumPos()) );

      log_debug(
            "mimeListSize=" << mimeListSize() <<
           " mimeListPos=" << mimeListPos() <<
           " indexPtrSize=" << urlPtrSize() <<
           " indexPtrPos=" << urlPtrPos() <<
           " indexSize=" << indexSize() <<
           " indexPos=" << indexPos() <<
           " clusterPtrSize=" << clusterPtrSize() <<
           " clusterPtrPos=" << clusterPtrPos() <<
           " clusterCount=" << clusterCount() <<
           " articleCount=" << articleCount() <<
           " articleCount=" << dirents.size() <<
           " urlPtrPos=" << header.getUrlPtrPos() <<
           " titleIdxPos=" << header.getTitleIdxPos() <<
           " clusterCount=" << header.getClusterCount() <<
           " clusterPtrPos=" << header.getClusterPtrPos() <<
           " checksumPos=" << checksumPos()
           );
    }

    void ZimCreatorImpl::write(const std::string& fname, const std::string& tmpfname)
    {
      std::ofstream zimfile(fname.c_str());
      Md5stream md5;
      Tee out(zimfile, md5);

      out << header;

      log_debug("after writing header - pos=" << zimfile.tellp());

      // write mime type list
      std::vector<std::string> oldMImeList;
      std::vector<std::string> newMImeList;
      std::vector<uint16_t> mapping;

      for (RMimeTypes::const_iterator it = rmimeTypes.begin(); it != rmimeTypes.end(); ++it)
      {
        oldMImeList.push_back(it->second);
        newMImeList.push_back(it->second);
      }

      mapping.resize(oldMImeList.size());
      std::sort(newMImeList.begin(), newMImeList.end());

      for (unsigned i=0; i<oldMImeList.size(); ++i)
      {
        for (unsigned j=0; j<newMImeList.size(); ++j)
        {
          if (oldMImeList[i] == newMImeList[j])
            mapping[i] = static_cast<uint16_t>(j);
        }
      }

      for (unsigned i=0; i<dirents.size(); ++i)
      {
        if (dirents[i].isArticle())
          dirents[i].setMimeType(mapping[dirents[i].getMimeType()]);
      }

      for (unsigned i=0; i<newMImeList.size(); ++i)
      {
        out << newMImeList[i] << '\0';
      }

      out << '\0';

      // write url ptr list

      offset_t off(indexPos());
      for (DirentsType::const_iterator it = dirents.begin(); it != dirents.end(); ++it)
      {
        char tmp_buff[sizeof(offset_type)];
        toLittleEndian(off.v, tmp_buff);
        out.write(tmp_buff, sizeof(offset_type));
        off += it->getDirentSize();
      }

      log_debug("after writing direntPtr - pos=" << out.tellp());

      // write title index

      for (ArticleIdxVectorType::const_iterator it = titleIdx.begin(); it != titleIdx.end(); ++it)
      {
        char tmp_buff[sizeof(article_index_type)];
        toLittleEndian(it->v, tmp_buff);
        out.write(tmp_buff, sizeof(article_index_type));
      }

      log_debug("after writing fileIdxList - pos=" << out.tellp());

      // write directory entries

      for (DirentsType::const_iterator it = dirents.begin(); it != dirents.end(); ++it)
      {
        out << *it;
        log_debug("write " << it->getTitle() << " dirent.size()=" << it->getDirentSize() << " pos=" << out.tellp());
      }

      log_debug("after writing dirents - pos=" << out.tellp());

      // write cluster offset list

      off += clusterOffsets.size() * sizeof(offset_type);
      for (OffsetsType::const_iterator it = clusterOffsets.begin(); it != clusterOffsets.end(); ++it)
      {
        offset_t o(off + *it);
        char tmp_buff[sizeof(offset_type)];
        toLittleEndian(o.v, tmp_buff);
        out.write(tmp_buff, sizeof(offset_type));
      }

      log_debug("after writing clusterOffsets - pos=" << out.tellp());

      // write cluster data

      if (!isEmpty)
      {
        std::ifstream blobsfile(tmpfname.c_str());
        out << blobsfile.rdbuf();
      }
      else
        log_warn("no data found");

      if (!out)
        throw std::runtime_error("failed to write zimfile");

      log_debug("after writing clusterData - pos=" << out.tellp());
      unsigned char digest[16];
      md5.getDigest(digest);
      zimfile.write(reinterpret_cast<const char*>(digest), 16);
    }

    zsize_t ZimCreatorImpl::mimeListSize() const
    {
      size_type ret = 1;
      for (RMimeTypes::const_iterator it = rmimeTypes.begin(); it != rmimeTypes.end(); ++it)
        ret += (it->second.size() + 1);
      return zsize_t(ret);
    }

    zsize_t ZimCreatorImpl::indexSize() const
    {
      size_type s = 0;

      for (DirentsType::const_iterator it = dirents.begin(); it != dirents.end(); ++it)
        s += it->getDirentSize();

      return zsize_t(s);
    }

    uint16_t ZimCreatorImpl::getMimeTypeIdx(const std::string& mimeType)
    {
      MimeTypes::const_iterator it = mimeTypes.find(mimeType);
      if (it == mimeTypes.end())
      {
        if (nextMimeIdx >= std::numeric_limits<uint16_t>::max())
          throw std::runtime_error("too many distinct mime types");
        mimeTypes[mimeType] = nextMimeIdx;
        rmimeTypes[nextMimeIdx] = mimeType;
        return nextMimeIdx++;
      }

      return it->second;
    }

    const std::string& ZimCreatorImpl::getMimeType(uint16_t mimeTypeIdx) const
    {
      RMimeTypes::const_iterator it = rmimeTypes.find(mimeTypeIdx);
      if (it == rmimeTypes.end())
        throw std::runtime_error("mime type index not found");
      return it->second;
    }

    ZimCreator::ZimCreator(bool verbose):
      impl(new ZimCreatorImpl(verbose))
    {};

    ZimCreator::~ZimCreator() = default;

    size_type ZimCreator::getMinChunkSize() const
    {
      return size_type(impl->getMinChunkSize());
    }

    void ZimCreator::setMinChunkSize(size_type s)
    {
      impl->setMinChunkSize(zsize_t(s));
    }

    void ZimCreator::create(const std::string& fname, ArticleSource& src)
    {
      impl->create(fname, src);
    }

  }
}
