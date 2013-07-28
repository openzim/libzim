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

#include <zim/writer/zimcreator.h>
#include <zim/fileheader.h>
#include <zim/cluster.h>
#include <zim/blob.h>
#include <zim/endian.h>
#include <algorithm>
#include <fstream>
#include <unistd.h>
#include <stdio.h>
#include <limits>
#include <stdexcept>
#include "config.h"
#include "arg.h"
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
    ZimCreator::ZimCreator(int& argc, char* argv[])
      : nextMimeIdx(0),
#ifdef ENABLE_LZMA
        compression(zimcompLzma)
#elif ENABLE_BZIP2
        compression(zimcompBzip2)
#elif ENABLE_ZLIB
        compression(zimcompZip)
#else
        compression(zimcompNone)
#endif
    {
      Arg<unsigned> minChunkSizeArg(argc, argv, "--min-chunk-size");
      if (minChunkSizeArg.isSet())
        minChunkSize = minChunkSizeArg;
      else
        minChunkSize = Arg<unsigned>(argc, argv, 's', 1024-64);

#ifdef ENABLE_ZLIB
      if (Arg<bool>(argc, argv, "--zlib"))
        compression = zimcompZip;
#endif
#ifdef ENABLE_BZIP2
      if (Arg<bool>(argc, argv, "--bzip2"))
        compression = zimcompBzip2;
#endif
#ifdef ENABLE_LZMA
      if (Arg<bool>(argc, argv, "--lzma"))
        compression = zimcompLzma;
#endif
    }

    void ZimCreator::create(const std::string& fname, ArticleSource& src)
    {
      isEmpty = true;

      std::string basename = fname;
      basename =  (fname.size() > 4 && fname.compare(fname.size() - 4, 4, ".zim") == 0)
                     ? fname.substr(0, fname.size() - 4)
                     : fname;
      log_debug("basename " << basename);

      INFO("create directory entries");
      createDirents(src);
      INFO(dirents.size() << " directory entries created");

      INFO("create title index");
      createTitleIndex(src);
      INFO(dirents.size() << " title index created");

      INFO("create clusters");
      createClusters(src, basename + ".tmp");
      INFO(clusterOffsets.size() << " clusters created");

      INFO("fill header");
      fillHeader(src);

      INFO("write zimfile");
      write(basename + ".zim", basename + ".tmp");

      ::remove((basename + ".tmp").c_str());

      INFO("ready");
    }

    void ZimCreator::createDirents(ArticleSource& src)
    {
      INFO("collect articles");

      const Article* article;
      while ((article = src.getNextArticle()) != 0)
      {
        Dirent dirent;
        dirent.setAid(article->getAid());
        dirent.setUrl(article->getNamespace(), article->getUrl());
        dirent.setTitle(article->getTitle());
        dirent.setParameter(article->getParameter());

        log_debug("article " << dirent.getLongUrl() << " fetched");

        if (article->isRedirect())
        {
          dirent.setRedirect(0);
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
          dirent.setArticle(getMimeTypeIdx(article->getMimeType()), 0, 0);
          dirent.setCompress(article->shouldCompress());
          log_debug("is article; mimetype " << dirent.getMimeType());
        }

        dirents.push_back(dirent);
      }

      // sort
      INFO("sort " << dirents.size() << " directory entries (aid)");
      std::sort(dirents.begin(), dirents.end(), compareAid);

      // remove invalid redirects
      INFO("remove invalid redirects from " << dirents.size() << " directory entries");
      DirentsType::size_type di = 0;
      while (di < dirents.size())
      {
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
      unsigned idx = 0;
      for (DirentsType::iterator di = dirents.begin(); di != dirents.end(); ++di)
        di->setIdx(idx++);

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

    }

    namespace
    {
      class CompareTitle
      {
          ZimCreator::DirentsType& dirents;

        public:
          explicit CompareTitle(ZimCreator::DirentsType& dirents_)
            : dirents(dirents_)
            { }
          bool operator() (size_type titleIdx1, size_type titleIdx2) const
          {
            Dirent d1 = dirents[titleIdx1];
            Dirent d2 = dirents[titleIdx2];
            return d1.getNamespace() < d2.getNamespace()
               || (d1.getNamespace() == d2.getNamespace()
                && d1.getTitle() < d2.getTitle());
          }
      };
    }

    void ZimCreator::createTitleIndex(ArticleSource& src)
    {
      titleIdx.resize(dirents.size());
      for (DirentsType::size_type n = 0; n < dirents.size(); ++n)
        titleIdx[n] = dirents[n].getIdx();

      CompareTitle compareTitle(dirents);
      std::sort(titleIdx.begin(), titleIdx.end(), compareTitle);
    }

    void ZimCreator::createClusters(ArticleSource& src, const std::string& tmpfname)
    {
      std::ofstream out(tmpfname.c_str());

      Cluster cluster;
      cluster.setCompression(compression);

      DirentsType::size_type count = 0, progress = 0;
      for (DirentsType::iterator di = dirents.begin(); out && di != dirents.end(); ++di, ++count)
      {
        while (progress < count * 100 / dirents.size() + 1)
        {
          INFO(progress << "% ready");
          progress += 10;
        }

        if (di->isRedirect())
          continue;

        Blob blob = src.getData(di->getAid());
        if (blob.size() > 0)
          isEmpty = false;

        if (di->isCompress())
        {
          di->setCluster(clusterOffsets.size(), cluster.count());
          cluster.addBlob(blob);
          if (cluster.size() >= minChunkSize * 1024)
          {
            log_info("compress cluster with " << cluster.count() << " articles, " << cluster.size() << " bytes; current title \"" << di->getTitle() << '\"');

            clusterOffsets.push_back(out.tellp());
            out << cluster;
            log_debug("cluster compressed");
            cluster.clear();
            cluster.setCompression(compression);
          }
        }
        else
        {
          if (cluster.count() > 0)
          {
            clusterOffsets.push_back(out.tellp());
            cluster.setCompression(compression);
            out << cluster;
            cluster.clear();
            cluster.setCompression(compression);
          }

          di->setCluster(clusterOffsets.size(), cluster.count());
          clusterOffsets.push_back(out.tellp());
          Cluster c;
          c.addBlob(blob);
          c.setCompression(zimcompNone);
          out << c;
        }
      }

      if (cluster.count() > 0)
      {
        clusterOffsets.push_back(out.tellp());
        cluster.setCompression(compression);
        out << cluster;
      }

      if (!out)
        throw std::runtime_error("failed to write temporary cluster file");

      clustersSize = out.tellp();
    }

    void ZimCreator::fillHeader(ArticleSource& src)
    {
      std::string mainAid = src.getMainPage();
      std::string layoutAid = src.getLayoutPage();

      log_debug("main aid=" << mainAid << " layout aid=" << layoutAid);

      header.setMainPage(std::numeric_limits<size_type>::max());
      header.setLayoutPage(std::numeric_limits<size_type>::max());

      if (!mainAid.empty() || !layoutAid.empty())
      {
        for (DirentsType::const_iterator di = dirents.begin(); di != dirents.end(); ++di)
        {
          if (mainAid == di->getAid())
          {
            log_debug("main idx=" << di->getIdx());
            header.setMainPage(di->getIdx());
          }

          if (layoutAid == di->getAid())
          {
            log_debug("layout idx=" << di->getIdx());
            header.setLayoutPage(di->getIdx());
          }
        }
      }

      header.setUuid( src.getUuid() );
      header.setArticleCount( dirents.size() );
      header.setUrlPtrPos( urlPtrPos() );
      header.setMimeListPos( mimeListPos() );
      header.setTitleIdxPos( titleIdxPos() );
      header.setClusterCount( clusterOffsets.size() );
      header.setClusterPtrPos( clusterPtrPos() );
      header.setChecksumPos( checksumPos() );

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

    void ZimCreator::write(const std::string& fname, const std::string& tmpfname)
    {
      std::ofstream zimfile(fname.c_str());
      Md5stream md5;
      Tee out(zimfile, md5);

      out << header;

      log_debug("after writing header - pos=" << zimfile.tellp());

      // write mime type list

      for (RMimeTypes::const_iterator it = rmimeTypes.begin(); it != rmimeTypes.end(); ++it)
      {
        out << it->second << '\0';
      }

      out << '\0';

      // write url ptr list

      offset_type off = indexPos();
      for (DirentsType::const_iterator it = dirents.begin(); it != dirents.end(); ++it)
      {
        offset_type ptr0 = fromLittleEndian<offset_type>(&off);
        out.write(reinterpret_cast<const char*>(&ptr0), sizeof(ptr0));
        off += it->getDirentSize();
      }

      log_debug("after writing direntPtr - pos=" << out.tellp());

      // write title index

      for (SizeVectorType::const_iterator it = titleIdx.begin(); it != titleIdx.end(); ++it)
      {
        size_type v = fromLittleEndian<size_type>(&*it);
        out.write(reinterpret_cast<const char*>(&v), sizeof(v));
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
        offset_type o = (off + *it);
        offset_type ptr0 = fromLittleEndian<offset_type>(&o);
        out.write(reinterpret_cast<const char*>(&ptr0), sizeof(ptr0));
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

    offset_type ZimCreator::mimeListSize() const
    {
      offset_type ret = 1;
      for (RMimeTypes::const_iterator it = rmimeTypes.begin(); it != rmimeTypes.end(); ++it)
        ret += (it->second.size() + 1);
      return ret;
    }

    offset_type ZimCreator::indexSize() const
    {
      offset_type s = 0;

      for (DirentsType::const_iterator it = dirents.begin(); it != dirents.end(); ++it)
        s += it->getDirentSize();

      return s;
    }

    uint16_t ZimCreator::getMimeTypeIdx(const std::string& mimeType)
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

    const std::string& ZimCreator::getMimeType(uint16_t mimeTypeIdx) const
    {
      RMimeTypes::const_iterator it = rmimeTypes.find(mimeTypeIdx);
      if (it == rmimeTypes.end())
        throw std::runtime_error("mime type index not found");
      return it->second;
    }

  }
}
