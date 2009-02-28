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

#ifndef ZIM_FILES_H
#define ZIM_FILES_H

#include <zim/file.h>
#include <map>
#include <zim/stringlessignorecase.h>

namespace zim
{
  class Files
  {
    public:
      typedef std::map<std::string, File, zim::StringLessIgnoreCase<std::string> > FilesType;

    private:
      FilesType files;
      FilesType fixFiles;

      static Article getArticle(FilesType& files, char ns, const QUnicodeString& url);

    public:
      typedef FilesType::iterator iterator;
      typedef FilesType::const_iterator const_iterator;

      Files() { }
      explicit Files(const std::string& dir, const std::string& fixdir = std::string());

      void addFile(const std::string& fname)  { files[fname] = File(fname); }
      void addFile(const std::string& fname, const File& file )  { files[fname] = file; }
      void addFiles(const std::string& dir, unsigned maxdepth = 2);
      void addFixFile(const std::string& fname)  { fixFiles[fname] = File(fname); }
      void addFixFile(const std::string& fname, const File& file )  { fixFiles[fname] = file; }
      void addFixFiles(const std::string& dir, unsigned maxdepth = 2);

      Files getFiles(char ns);
      File getFirstFile(char ns);
      const FilesType& getFixFiles() const    { return fixFiles; }

      Article getArticle(char ns, const QUnicodeString& url);
      Article getArticle(const std::string& file, char ns, const QUnicodeString& url);
      Article getBestArticle(const Article& article);
      Article getFixArticle(char ns, const QUnicodeString& url);

      iterator begin()                 { return files.begin(); }
      iterator end()                   { return files.end(); }
      const_iterator begin() const     { return files.begin(); }
      const_iterator end() const       { return files.end(); }
      bool empty()                     { return files.empty(); }
  };
}

#endif // ZIM_FILES_H
