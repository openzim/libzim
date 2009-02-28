/*
 * Copyright (C) 2007 Tommi Maekitalo
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

#include <zim/files.h>
#include <zim/article.h>
#include <zim/error.h>
#include <cxxtools/dir.h>
#include <cxxtools/log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

log_define("zim.file.files");

namespace zim
{
  namespace
  {
    void addFiles_(Files::FilesType& files, const std::string& dir, unsigned maxdepth)
    {
      if (dir.empty())
        return;

      try
      {
        log_trace("addFiles_ " << dir);
        cxxtools::Dir d(dir.c_str());
        for (cxxtools::Dir::const_iterator it = d.begin(); it != d.end(); ++it)
        {
          std::string fname = *it;
          log_trace("process " << fname);
          if (fname.size() > 4 && fname.compare(fname.size() - 5, 5, ".zim") == 0)
          {
            log_debug("file \"" << fname << "\" found");
            try
            {
              files[fname] = File(dir + '/' + fname);
            }
            catch (const ZenoFileFormatError& e)
            {
              log_error('"' << fname << "\" is no zim file: " << e.what());
            }
          }
          else if (fname.size() > 0 && fname[0] != '.' && maxdepth > 1)
          {
            addFiles_(files, dir + '/' + *it, maxdepth - 1);
          }
        }
      }
      catch (const cxxtools::DirectoryNotFound&)
      {
        log_debug("directory \"" << dir << "\" not found");
      }
    }
  }

  Files::Files(const std::string& dir, const std::string& fixdir)
  {
    if (!dir.empty())
      addFiles(dir);

    log_debug("fixdir=" << fixdir);
    if (!fixdir.empty())
    {
      FilesType::iterator f = files.find(fixdir);
      struct stat st;
      if (f != files.end())
      {
        log_debug("fix file=" << fixdir);
        addFixFile(fixdir, f->second);
      }
      else if (::stat(fixdir.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
      {
        std::string fname = fixdir;
        std::string::size_type p = fname.find_last_of('/');
        if (p != std::string::npos)
          fname.erase(0, p + 1);
        log_debug("fix file=" << fname << " (" << fixdir << ')');
        fixFiles[fname] = File(fixdir);
      }
      else
      {
        log_debug("add fix files from directory " << fixdir);
        addFixFiles(fixdir);
      }
    }
  }

  void Files::addFiles(const std::string& dir, unsigned maxdepth)
  {
    log_debug("search for zim-files in directory " << dir);
    addFiles_(files, dir, maxdepth);
    log_debug(files.size() << " zimfiles active");
  }

  void Files::addFixFiles(const std::string& dir, unsigned maxdepth)
  {
    log_debug("search for additional zim-files in directory " << dir);
    addFiles_(fixFiles, dir, maxdepth);
    log_debug(files.size() << " zimfiles");
  }

  Files Files::getFiles(char ns)
  {
    Files ret;
    for (iterator it = begin(); it != end(); ++it)
      if (it->second.hasNamespace(ns))
        ret.addFile(it->first, it->second);
    return ret;
  }

  File Files::getFirstFile(char ns)
  {
    Files ret;
    for (iterator it = begin(); it != end(); ++it)
      if (it->second.hasNamespace(ns))
        return it->second;
    return File();
  }

  Article Files::getArticle(FilesType& files, char ns, const QUnicodeString& url)
  {
    log_debug("getArticle(files, '" << ns << "', \"" << url << "\")");
    Article ret;
    for (iterator it = files.begin(); it != files.end(); ++it)
    {
      log_debug("search in " << it->first);
      Article article = it->second.getArticle(ns, url);
      if (article)
      {
        if (ret.getRedirectFlag())
        {
          log_debug("redirect article found - return it");
          return ret;
        }
        else if (ret.getData().size() <= article.getData().size())
        {
          log_debug("article found in " << it->first);
          ret = article;
        }
        else
        {
          log_debug("article size " << article.getData().size() << " smaller than previous " << ret.getData().size());
        }
      }
      else
      {
        log_debug("no article");
      }
    }

    return ret;
  }

  Article Files::getArticle(char ns, const QUnicodeString& url)
  {
    log_debug("getArticle('" << ns << "', \"" << url << "\")");
    Article ret = getArticle(files, ns, url);

    if (!ret)
      ret = getFixArticle(ns, url);

    if (!ret)
      log_debug("article not found");

    return ret;
  }

  Article Files::getArticle(const std::string& file, char ns, const QUnicodeString& url)
  {
    log_debug("getArticle(\"" << file << "\", '" << ns << "', \"" << url << "\")");

    iterator it = files.find(file);
    if (it == end())
    {
      log_debug("file \"" << file << "\" not found");
      return Article();
    }

    log_debug("file \"" << file << "\" found");
    Article article = it->second.getArticle(ns, url);

    if (!article)
      article = getFixArticle(ns, url);

    if (article)
    {
      log_debug("article found");
      return getBestArticle(article);
    }

    log_debug("article not found");
    return article;
  }

  Article Files::getBestArticle(const Article& article)
  {
    log_debug("getBestArticle");

    Article ret = article;
    for (iterator it = begin(); it != end(); ++it)
    {
      log_debug("search in " << it->first);
      Article a = it->second.getArticle(article.getNamespace(), article.getTitle());
      if (a
        && a.getLibraryMimeType() == article.getLibraryMimeType()
        && ret.getData().size() < a.getData().size())
      {
        log_debug("article found in " << it->first);
        ret = a;
      }
    }

    if (!ret)
      log_debug("article not found");
    return ret;
  }

  Article Files::getFixArticle(char ns, const QUnicodeString& url)
  {
    log_debug("getFixArticle('" << ns << "', \"" << url << "\")");
    return getArticle(fixFiles, ns, url);
  }

}
