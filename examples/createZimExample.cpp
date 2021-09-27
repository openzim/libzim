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

#include <zim/writer/contentProvider.h>
#include <zim/writer/creator.h>
#include <zim/blob.h>

class TestItem : public zim::writer::Item
{
    std::string _id;
    std::string _data;

  public:
    TestItem()  { }
    explicit TestItem(const std::string& id);
    virtual ~TestItem() = default;

    virtual std::string getPath() const;
    virtual std::string getTitle() const;
    virtual std::string getMimeType() const;

    virtual std::unique_ptr<zim::writer::ContentProvider> getContentProvider() const;
};

TestItem::TestItem(const std::string& id)
  : _id(id)
{
  std::ostringstream data;
  data << "this is item " << id << std::endl;
  _data = data.str();
}

std::string TestItem::getPath() const
{
  return std::string("A/") + _id;
}

std::string TestItem::getTitle() const
{
  return _id;
}

std::string TestItem::getMimeType() const
{
  return "text/plain";
}

std::unique_ptr<zim::writer::ContentProvider> TestItem::getContentProvider() const
{
  return std::unique_ptr<zim::writer::ContentProvider>(new zim::writer::StringProvider(_data));
}

int main(int argc, char* argv[])
{
  unsigned max = 16;
  try {
    zim::writer::Creator c;
    c.configVerbose(false).configCompression(zim::Compression::Zstd);
    c.startZimCreation("foo.zim");
    for (unsigned n = 0; n < max; ++n)
    {
      std::ostringstream id;
      id << (n + 1);
      auto article = std::make_shared<TestItem>(id.str());
      c.addItem(article);
    }
    c.setMainPath("A/0");
    c.finishZimCreation();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

