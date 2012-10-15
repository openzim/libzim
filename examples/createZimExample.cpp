/*
 * Copyright (C) 2012 Tommi Maekitalo
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
#include <vector>
#include <zim/writer/zimcreator.h>
#include <zim/blob.h>

class TestArticle : public zim::writer::Article
{
    std::string _id;
    std::string _data;

  public:
    TestArticle()  { }
    explicit TestArticle(const std::string& id);

    virtual std::string getAid() const;
    virtual char getNamespace() const;
    virtual std::string getUrl() const;
    virtual std::string getTitle() const;
    virtual bool isRedirect() const;
    virtual std::string getMimeType() const;
    virtual std::string getRedirectAid() const;

    zim::Blob data()
    { return zim::Blob(&_data[0], _data.size()); }
};

TestArticle::TestArticle(const std::string& id)
  : _id(id)
{
  std::ostringstream data;
  data << "this is article " << id << std::endl;
  _data = data.str();
}

std::string TestArticle::getAid() const
{
  return _id;
}

char TestArticle::getNamespace() const
{
  return 'A';
}

std::string TestArticle::getUrl() const
{
  return _id;
}

std::string TestArticle::getTitle() const
{
  return _id;
}

bool TestArticle::isRedirect() const
{
  return false;
}

std::string TestArticle::getMimeType() const
{
  return "text/plain";
}

std::string TestArticle::getRedirectAid() const
{
  return "";
}

class TestArticleSource : public zim::writer::ArticleSource
{
    std::vector<TestArticle> _articles;
    unsigned _next;

  public:
    explicit TestArticleSource(unsigned max = 16);

    virtual const zim::writer::Article* getNextArticle();
    virtual zim::Blob getData(const std::string& aid);
};

TestArticleSource::TestArticleSource(unsigned max)
  : _next(0)
{
  _articles.resize(max);
  for (unsigned n = 0; n < max; ++n)
  {
    std::ostringstream id;
    id << (n + 1);
    _articles[n] = TestArticle(id.str());
  }
}

const zim::writer::Article* TestArticleSource::getNextArticle()
{
  if (_next >= _articles.size())
    return 0;

  unsigned n = _next++;

  return &_articles[n];
}

zim::Blob TestArticleSource::getData(const std::string& aid)
{
  unsigned n;
  std::istringstream s(aid);
  s >> n;
  return _articles[n-1].data();
}

int main(int argc, char* argv[])
{
  try
  {
    zim::writer::ZimCreator c(argc, argv);
    TestArticleSource src;
    c.create("foo.zim", src);
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

