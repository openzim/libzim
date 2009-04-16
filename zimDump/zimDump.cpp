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
    explicit ZimDumper(const char* fname)
      : file(fname),
        pos(file.begin()),
        verbose(false)
      { }

    void setVerbose(bool sw = true)  { verbose = sw; }
    
    void printInfo();
    void printNsInfo(char ch);
    void locateArticle(zim::size_type idx);
    void findArticle(char ns, const char* url, bool collate);
    void dumpArticle();
    void printPage();
    void listArticles(bool info, bool extra);
    void listArticle(const zim::Article& article, bool extra);
    void listArticle(bool extra)
      { listArticle(*pos, extra); }
    void dumpFiles(const std::string& directory);
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
               "index ptr pos: " << file.getFileheader().getIndexPtrPos() << "\n"
               "cluster count: " << file.getFileheader().getClusterCount() << "\n"
               "cluster ptr pos: " << file.getFileheader().getClusterPtrPos() << "\n";

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

void ZimDumper::findArticle(char ns, const char* url, bool collate)
{
  log_debug("findArticle(" << ns << ", " << url << ", " << collate << ')');
  pos = file.find(ns, zim::QUnicodeString::fromUtf8(url), collate);
  log_debug("findArticle(" << ns << ", " << url << ", " << collate << ") => idx=" << pos.getIndex());
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

void ZimDumper::listArticles(bool info, bool extra)
{
  log_trace("listArticles(" << info << ", " << extra << ") verbose=" << verbose);

  for (zim::File::const_iterator it = pos; it != file.end(); ++it)
  {
    if (info)
      listArticle(*it, extra);
    else
      std::cout << it->getUrl() << '\n';
  }
}

void ZimDumper::listArticle(const zim::Article& article, bool extra)
{
  std::cout <<
      "title: "           << article.getTitle() << "\n"
    "\tidx:             " << article.getIndex() << "\n"
    "\tnamespace:       " << article.getNamespace() << "\n"
    "\tredirect:        " << article.isRedirect() << "\n";

  if (article.isRedirect())
  {
    std::cout <<
      "\tredirect index:  " << article.getRedirectIndex() << "\n";
  }
  else
  {
    std::cout <<
      "\tmime-type:       " << article.getLibraryMimeType() << "\n"
      "\tarticle size:    " << article.getArticleSize() << "\n";

    if (verbose)
    {
      std::cout <<
      "\tcluster number:  " << article.getDirent().getClusterNumber() << "\n"
      "\tcluster count:   " << article.getCluster().count() << "\n"
      "\tcluster size:    " << article.getCluster().size() << "\n"
      "\tcluster offset:  " << file.getClusterOffset(article.getDirent().getClusterNumber()) << "\n"
      "\tblob number:     " << article.getDirent().getBlobNumber() << "\n"
      "\tcompression:     " << static_cast<unsigned>(article.getCluster().getCompression()) << "\n";
    }
  }

  if (extra)
  {
    std::string parameter = article.getParameter();
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
      unsigned len = s.get(); // read length byte
      std::cout << "len=" << len;

      zim::IZIntStream in(s);
      unsigned val;
      while (in.get(val))
        std::cout << '\t' << val;
    }

    std::cout << std::endl;
  }
}

void ZimDumper::dumpFiles(const std::string& directory)
{
  std::set<char> ns;
  for (zim::File::const_iterator it = pos; it != file.end(); ++it)
  {
    std::string d = directory + '/' + it->getNamespace();
    if (ns.find(it->getNamespace()) == ns.end())
      ::mkdir(d.c_str(), 0777);
    std::string t = it->getTitle().getValue();
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
    cxxtools::Arg<bool> list(argc, argv, 'l');
    cxxtools::Arg<zim::size_type> indexOffset(argc, argv, 'o');
    cxxtools::Arg<bool> extra(argc, argv, 'x');
    cxxtools::Arg<char> ns(argc, argv, 'n', 'A');  // namespace
    cxxtools::Arg<bool> collate(argc, argv, 'c');
    cxxtools::Arg<const char*> dumpAll(argc, argv, 'D');
    cxxtools::Arg<bool> verbose(argc, argv, 'v');

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
                   "  -f url    find article\n"
                   "  -l        list articles\n"
                   "  -o idx    locate article\n"
                   "  -x        print extra parameters\n"
                   "  -n ns     specify namespace (default 'A')\n"
                   "  -D dir    dump all files into directory\n"
                   "  -v        verbose (print uncompressed length of articles when -i is set)\n"
                   "\n"
                   "examples:\n"
                   "  " << argv[0] << " -F wikipedia.zim\n"
                   "  " << argv[0] << " -l wikipedia.zim\n"
                   "  " << argv[0] << " -f A/Auto -i wikipedia.zim\n"
                   "  " << argv[0] << " -f A/Auto -d wikipedia.zim\n"
                   "  " << argv[0] << " -f A/Auto -l wikipedia.zim\n"
                   "  " << argv[0] << " -f A/Auto -l -i wikipedia.zim\n"
                   "  " << argv[0] << " -o 123159 -l -i wikipedia.zim\n"
                 << std::flush;
      return -1;
    }

    // initalize app
    ZimDumper app(argv[1]);
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
      app.findArticle(ns, find, collate);

    // dump files
    if (dumpAll.isSet())
      app.dumpFiles(dumpAll.getValue());

    // print requested info
    if (data)
      app.dumpArticle();
    else if (page)
      app.printPage();
    else if (list)
      app.listArticles(info, extra);
    else if (info)
      app.listArticle(extra);
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return -2;
  }
  return 0;
}

