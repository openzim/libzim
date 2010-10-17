/*
 * Copyright (C) 2006 Tommi Maekitalo
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

#include <iostream>
#include <sstream>
#include <fstream>
#include <set>
#include <zim/file.h>
#include <zim/fileiterator.h>
#include <zim/zintstream.h>
#include <cxxtools/arg.h>
#include <cxxtools/loginit.h>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>

log_define("zim.dumper")

class ZimDumper
{
    zim::File file;
    zim::File::const_iterator pos;
    bool verbose;

  public:
    ZimDumper(const char* fname, bool titleSort)
      : file(fname),
        pos(titleSort ? file.beginByTitle() : file.begin()),
        verbose(false)
      { }

    void setVerbose(bool sw = true)  { verbose = sw; }
    
    void printInfo();
    void printNsInfo(char ch);
    void locateArticle(zim::size_type idx);
    void findArticle(char ns, const char* expr, bool title);
    void findArticleByUrl(const std::string& url);
    void dumpArticle();
    void dumpIndex();
    void printPage();
    void listArticles(bool info, bool listTable, bool extra);
    void listArticle(const zim::Article& article, bool extra);
    void listArticleT(const zim::Article& article, bool extra);
    void listArticle(bool extra)
      { listArticle(*pos, extra); }
    void listArticleT(bool extra)
      { listArticleT(*pos, extra); }
    void dumpFiles(const std::string& directory);
    void verifyChecksum();
};

void ZimDumper::printInfo()
{
  std::cout << "count-articles: " << file.getCountArticles() << "\n";
  if (verbose)
  {
    std::string ns = file.getNamespaces();
    std::cout << "namespaces: " << ns << '\n';
    for (std::string::const_iterator it = ns.begin(); it != ns.end(); ++it)
      std::cout << "namespace " << *it << " size: " << file.getNamespaceCount(*it) << '\n';
  }
  std::cout << "uuid: " << file.getFileheader().getUuid() << "\n"
               "article count: " << file.getFileheader().getArticleCount() << "\n"
               "mime list pos: " << file.getFileheader().getMimeListPos() << "\n"
               "url ptr pos: " << file.getFileheader().getUrlPtrPos() << "\n"
               "title idx pos: " << file.getFileheader().getTitleIdxPos() << "\n"
               "cluster count: " << file.getFileheader().getClusterCount() << "\n"
               "cluster ptr pos: " << file.getFileheader().getClusterPtrPos() << "\n";
  if (file.getFileheader().hasChecksum())
    std::cout << 
               "checksum pos: " << file.getFileheader().getChecksumPos() << "\n"
               "checksum: " << file.getChecksum() << "\n";
  else
    std::cout << 
               "no checksum\n";

  if (file.getFileheader().hasMainPage())
    std::cout << "main page: " << file.getFileheader().getMainPage() << "\n";
  else
    std::cout << "main page: " << "-\n";

  if (file.getFileheader().hasLayoutPage())
    std::cout << "layout page: " << file.getFileheader().getLayoutPage() << "\n";
  else
    std::cout << "layout page: " << "-\n";

  std::cout.flush();
}

void ZimDumper::printNsInfo(char ch)
{
  std::cout << "namespace " << ch << "\n"
               "lower bound idx: " << file.getNamespaceBeginOffset(ch) << "\n"
               "upper bound idx: " << file.getNamespaceEndOffset(ch) << std::endl;
}

void ZimDumper::locateArticle(zim::size_type idx)
{
  log_debug("locateArticle(" << idx << ')');
  pos = zim::File::const_iterator(&file, idx);
}

void ZimDumper::findArticle(char ns, const char* expr, bool title)
{
  log_debug("findArticle(" << ns << ", " << expr << ')');
  if (title)
    pos = file.findByTitle(ns, expr);
  else
    pos = file.find(ns, expr);
  log_debug("findArticle(" << ns << ", " << expr << ") => idx=" << pos.getIndex());
}

void ZimDumper::findArticleByUrl(const std::string& url)
{
    pos = file.find(url);
}

void ZimDumper::printPage()
{
  log_trace("print page");
  std::cout << pos->getPage() << std::flush;
}

void ZimDumper::dumpArticle()
{
  log_trace("dump article");
  std::cout << pos->getData() << std::flush;
}

void ZimDumper::dumpIndex()
{
  log_trace("dump index");

  if (pos->getNamespace() == 'X')
  {
    // prepare parameter stream
    std::istringstream paramstream(pos->getParameter());
    zim::ZIntStream parameter(paramstream);

    // read flags
    unsigned flags, off=0;
    parameter.get(flags);
    if (!parameter)
      throw std::runtime_error("invalid index parameter data");

    // process categories
    for (unsigned c = 0, flag = 1; c < 4; ++c, flag <<= 1)
    {
      if (!(flags & flag))
        continue;  // category empty

      unsigned len, idx, wpos;
      parameter.get(len).get(idx).get(wpos);
      if (!parameter)
        throw std::runtime_error("invalid index parameter data");

      if (verbose)
        std::cout << 'c' << c << "\tidx=" << idx << "\tpos=" << wpos << std::endl;
      else
        std::cout << 'c' << c << '\t' << idx << ';' << wpos;

      // prepare data stream
      zim::Blob data = pos->getData();

      if (off + len > data.size())
        throw std::runtime_error("invalid index data");

      std::string data_s(data.data() + off, len);
      std::istringstream stream(data_s);
      zim::ZIntStream in(stream);

      off += len;

      unsigned lastidx = 0, lastpos = 0;
      while (in.get(idx).get(wpos))
      {
        unsigned oidx = idx;
        unsigned owpos = wpos;
        if (idx == 0)
        {
          idx = lastidx;
          lastpos = (wpos += lastpos);
        }
        else
        {
          lastidx = (idx += lastidx);
          lastpos = wpos;
        }

        if (verbose)
          std::cout << 'c' << c << "\tidx=" << oidx << " => " << idx << "\tpos=" << owpos << " => " << wpos << std::endl;
        else
          std::cout << '\t' << idx << ';' << wpos;
      }

      if (!verbose)
        std::cout << std::endl;
    }

  }
  else
    std::cout << "no index article\n";
}

void ZimDumper::listArticles(bool info, bool listTable, bool extra)
{
  log_trace("listArticles(" << info << ", " << extra << ") verbose=" << verbose);

  for (zim::File::const_iterator it = pos; it != file.end(); ++it)
  {
    if (listTable)
      listArticleT(*it, extra);
    else if (info)
      listArticle(*it, extra);
    else
      std::cout << it->getUrl() << '\n';
  }
}

void ZimDumper::listArticle(const zim::Article& article, bool extra)
{
  zim::Dirent dirent = article.getDirent();

  std::cout <<
      "url: "             << dirent.getUrl() << "\n"
    "\ttitle:           " << dirent.getTitle() << "\n"
    "\tidx:             " << article.getIndex() << "\n"
    "\tnamespace:       " << dirent.getNamespace() << "\n"
    "\tredirect:        " << dirent.isRedirect() << "\n";

  if (dirent.isRedirect())
  {
    std::cout <<
      "\tredirect index:  " << dirent.getRedirectIndex() << "\n";
  }
  else
  {
    std::cout <<
      "\tmime-type:       " << article.getMimeType() << "\n"
      "\tarticle size:    " << article.getArticleSize() << "\n";

    if (verbose)
    {
      zim::Cluster cluster = article.getCluster();

      std::cout <<
      "\tcluster number:  " << dirent.getClusterNumber() << "\n"
      "\tcluster count:   " << cluster.count() << "\n"
      "\tcluster size:    " << cluster.size() << "\n"
      "\tcluster offset:  " << file.getClusterOffset(dirent.getClusterNumber()) << "\n"
      "\tblob number:     " << dirent.getBlobNumber() << "\n"
      "\tcompression:     ";
      switch (cluster.getCompression())
      {
        case zim::zimcompDefault: std::cout << "default"; break;
        case zim::zimcompNone:    std::cout << "none"; break;
        case zim::zimcompZip:     std::cout << "zip"; break;
        case zim::zimcompBzip2:   std::cout << "bzip2"; break;
        case zim::zimcompLzma:    std::cout << "lzma"; break;
        default:                  std::cout << "unknown (" << static_cast<unsigned>(cluster.getCompression()) << ')'; break;
      }
      std::cout << "\n";
    }
  }

  if (extra)
  {
    std::string parameter = dirent.getParameter();
    std::cout << "\textra:           ";
    static char hexdigit[] = "0123456789abcdef";
    for (std::string::const_iterator it = parameter.begin(); it != parameter.end(); ++it)
    {
      unsigned val = static_cast<unsigned>(static_cast<unsigned char>(*it));
      std::cout << hexdigit[val >> 4] << hexdigit[val & 0xf] << ' ';
    }
    std::cout << ':';

    if (parameter.size() > 1)
    {
      std::istringstream s(parameter);

      zim::ZIntStream in(s);
      unsigned val;
      while (in.get(val))
        std::cout << '\t' << val;
    }

    std::cout << std::endl;
  }
}

void ZimDumper::listArticleT(const zim::Article& article, bool extra)
{
  zim::Dirent dirent = article.getDirent();

  std::cout << dirent.getNamespace()
    << '\t' << dirent.getUrl()
    << '\t' << dirent.getTitle()
    << '\t' << article.getIndex()
    << '\t' << dirent.isRedirect();

  if (dirent.isRedirect())
  {
    std::cout << '\t' << dirent.getRedirectIndex();
  }
  else
  {
    std::cout << '\t' << dirent.getMimeType()
              << '\t' << article.getArticleSize();

    if (verbose)
    {
      zim::Cluster cluster = article.getCluster();

      std::cout << '\t' << dirent.getClusterNumber()
                << '\t' << cluster.count()
                << '\t' << cluster.size()
                << '\t' << file.getClusterOffset(dirent.getClusterNumber())
                << '\t' << dirent.getBlobNumber()
                << '\t' << static_cast<unsigned>(cluster.getCompression());
    }
  }

  if (extra)
  {
    std::string parameter = dirent.getParameter();
    std::cout << '\t';
    static char hexdigit[] = "0123456789abcdef";
    for (std::string::const_iterator it = parameter.begin(); it != parameter.end(); ++it)
    {
      unsigned val = static_cast<unsigned>(static_cast<unsigned char>(*it));
      std::cout << hexdigit[val >> 4] << hexdigit[val & 0xf] << '\t';
    }

    if (parameter.size() > 1)
    {
      std::istringstream s(parameter);

      zim::ZIntStream in(s);
      unsigned val;
      while (in.get(val))
        std::cout << '\t' << val;
    }

  }

  std::cout << std::endl;
}

void ZimDumper::dumpFiles(const std::string& directory)
{
  ::mkdir(directory.c_str(), 0777);

  std::set<char> ns;
  for (zim::File::const_iterator it = pos; it != file.end(); ++it)
  {
    std::string d = directory + '/' + it->getNamespace();
    if (ns.find(it->getNamespace()) == ns.end())
      ::mkdir(d.c_str(), 0777);
    std::string t = it->getTitle();
    std::string::size_type p;
    while ((p = t.find('/')) != std::string::npos)
      t.replace(p, 1, "%2f");
    std::string f = d + '/' + t;
    std::ofstream out(f.c_str());
    out << it->getData();
    if (!out)
      throw std::runtime_error("error writing file " + f);
  }
}

void ZimDumper::verifyChecksum()
{
  if (file.verify())
    std::cout << "checksum ok" << std::endl;
  else
    std::cout << "no checksum" << std::endl;
}

int main(int argc, char* argv[])
{
  try
  {
    log_init();

    cxxtools::Arg<bool> fileinfo(argc, argv, 'F');
    cxxtools::Arg<char> nsinfo(argc, argv, 'N');
    cxxtools::Arg<bool> info(argc, argv, 'i');
    cxxtools::Arg<bool> data(argc, argv, 'd');
    cxxtools::Arg<bool> page(argc, argv, 'p');
    cxxtools::Arg<const char*> find(argc, argv, 'f');
    cxxtools::Arg<const char*> url(argc, argv, 'u');
    cxxtools::Arg<bool> list(argc, argv, 'l');
    cxxtools::Arg<bool> tableList(argc, argv, 'L');
    cxxtools::Arg<zim::size_type> indexOffset(argc, argv, 'o');
    cxxtools::Arg<bool> extra(argc, argv, 'x');
    cxxtools::Arg<char> ns(argc, argv, 'n', 'A');  // namespace
    cxxtools::Arg<const char*> dumpAll(argc, argv, 'D');
    cxxtools::Arg<bool> verbose(argc, argv, 'v');
    cxxtools::Arg<bool> zint(argc, argv, 'Z');
    cxxtools::Arg<bool> titleSort(argc, argv, 't');
    cxxtools::Arg<bool> verifyChecksum(argc, argv, 'C');

    if (argc <= 1)
    {
      std::cerr << "usage: " << argv[0] << " [options] zimfile\n"
                   "\n"
                   "options:\n"
                   "  -F        print fileinfo\n"
                   "  -N ns     print info about namespace\n"
                   "  -i        print info about articles\n"
                   "  -d        print data of articles\n"
                   "  -p        print page\n"
                   "  -f title  find article\n"
                   "  -u url    find article by url\n"
                   "  -t        sort (and find) articles by title instead of url\n"
                   "  -l        list articles\n"
                   "  -L        list articles as table\n"
                   "  -o idx    locate article by index\n"
                   "  -x        print extra parameters\n"
                   "  -n ns     specify namespace (default 'A')\n"
                   "  -D dir    dump all files into directory\n"
                   "  -v        verbose (print uncompressed length of articles when -i is set)\n"
                   "                    (print namespaces with counts with -F)\n"
                   "  -Z        dump index data\n"
                   "  -C        verify checksum\n"
                   "\n"
                   "examples:\n"
                   "  " << argv[0] << " -F wikipedia.zim\n"
                   "  " << argv[0] << " -l wikipedia.zim\n"
                   "  " << argv[0] << " -f Auto -i wikipedia.zim\n"
                   "  " << argv[0] << " -f Auto -d wikipedia.zim\n"
                   "  " << argv[0] << " -f Auto -l wikipedia.zim\n"
                   "  " << argv[0] << " -f Auto -l -i -v wikipedia.zim\n"
                   "  " << argv[0] << " -o 123159 -l -i wikipedia.zim\n"
                 << std::flush;
      return -1;
    }

    // initalize app
    ZimDumper app(argv[1], titleSort);
    app.setVerbose(verbose);

    // global info
    if (fileinfo)
      app.printInfo();

    // namespace info
    if (nsinfo.isSet())
      app.printNsInfo(nsinfo);

    // locate article
    if (indexOffset.isSet())
      app.locateArticle(indexOffset);
    else if (find.isSet())
      app.findArticle(ns, find, titleSort);
    else if (url.isSet())
      app.findArticleByUrl(std::string(url));

    // dump files
    if (dumpAll.isSet())
      app.dumpFiles(dumpAll.getValue());

    // print requested info
    if (data)
      app.dumpArticle();
    else if (page)
      app.printPage();
    else if (list || tableList)
      app.listArticles(info, tableList, extra);
    else if (info)
      app.listArticle(extra);
    else if (zint)
      app.dumpIndex();

    if (verifyChecksum)
      app.verifyChecksum();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return -2;
  }
  return 0;
}

