/*
 * Copyright (C) 2020 Veloman Yunkan
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

#ifndef ZIM_TEST_TOOLS_H
#define ZIM_TEST_TOOLS_H


#include <string>
#include <vector>
#include <sys/types.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define LSEEK _lseeki64
#else
#include <unistd.h>
#define LSEEK lseek
#endif

#include "../src/buffer.h"
#include <limits.h>

#define ZIM_PRIVATE
#include <zim/archive.h>
#include <zim/search.h>
#include <zim/writer/creator.h>
#include <zim/writer/item.h>
#include <zim/writer/contentProvider.h>

namespace zim
{

namespace unittests
{

// TempFile is a utility class for working with temporary files in RAII fashion:
//
//   1. An empty temporary file is created (in the temporary file directory)
//      by the constructor.
//
//   2. The file can be filled with data via the file descriptor (returned
//      by the fd() member function).
//
//      -------------------------------------------------------------
//      | IMPORTANT!                                                |
//      |                                                           |
//      | The file descriptor must NOT be close()-ed. Under Windows |
//      | this will result in the file being removed.               |
//      -------------------------------------------------------------
//
//   3. The destructor automatically (closes and) removes the file
//
class TempFile
{
  int fd_;
  std::string path_;
#ifdef _WIN32
  wchar_t wpath_[MAX_PATH];
#endif
public:
  // Creates an empty file in the temporary directory (under Linux and friends
  // its path is read from the TMPDIR environment variable or defaults to /tmp)
  explicit TempFile(const char* name);

  TempFile(const TempFile& ) = delete;
  void operator=(const TempFile& ) = delete;

  // Closes and removes the file
  ~TempFile();

  // Close the file descriptor if opened
  void close();

  // File descriptor
  // Important! It must NOT be close()-ed
  int fd();

  // Absolute path of the file
  std::string path() const { return path_; }
};

template<typename T>
std::string to_string(const T& value)
{
  std::ostringstream ss;
  ss << value;
  return ss.str();
}

std::unique_ptr<TempFile>
makeTempFile(const char* name, const std::string& content);


template<typename T>
zim::Buffer write_to_buffer(const T& object, const std::string& tail="")
{
  TempFile tmpFile("test_temp_file");
  const auto tmp_fd = tmpFile.fd();
  object.write(tmp_fd);
  write(tmp_fd, tail.data(), tail.size());
  size_type size = LSEEK(tmp_fd, 0, SEEK_END);

  auto buf = zim::Buffer::makeBuffer(zim::zsize_t(size));
  LSEEK(tmp_fd, 0, SEEK_SET);
  char* p = const_cast<char*>(buf.data());
  while ( size != 0 ) {
    const auto size_to_read = std::min(size, size_type{1024*1024});
    const auto n = read(tmp_fd, p, size_to_read);
    if ( n == -1 )
      throw std::runtime_error("Cannot read " + tmpFile.path());
    p += n;
    size -= n;
  }
  return buf;
}

struct TestFile {
  TestFile(const std::string& dataDir, const std::string& category, const std::string& filename);
  const std::string filename;
  const std::string category;
  const std::string path;
};

const std::vector<TestFile> getDataFilePath(const std::string& filename, const std::string& category = "");

// Helper class to create temporary zim and remove it once the test is done
class TempZimArchive : zim::unittests::TempFile {
  public:
    explicit TempZimArchive(const char* tempPath) : zim::unittests::TempFile {tempPath} {}
    zim::Archive createZimFromTitles(std::vector<std::string> titles);
    zim::Archive createZimFromContent(std::vector<std::vector<std::string>> contents);
    const std::string getPath();
};

enum class IsFrontArticle {
  YES,
  NO,
  DEFAULT
};

class TestItem : public zim::writer::Item {
  public:
    TestItem(
        const std::string& path,
        const std::string& mimetype = "text/html",
        const std::string& title = "Test Item",
        const std::string& content = "foo",
        IsFrontArticle frontArticle = IsFrontArticle::DEFAULT) :
      path(path),
      title(title),
      content(content),
      mimetype(mimetype),
      frontArticle(frontArticle)
    {}
    virtual ~TestItem() = default;

    virtual std::string getPath() const { return path; };
    virtual std::string getTitle() const { return title; };
    virtual std::string getMimeType() const { return mimetype; };
    virtual zim::writer::Hints getHints() const {
      switch (frontArticle) {
        case IsFrontArticle::YES:
          return zim::writer::Hints{{zim::writer::FRONT_ARTICLE, 1}};
        case IsFrontArticle::NO:
          return zim::writer::Hints{{zim::writer::FRONT_ARTICLE, 0}};
        default:
          return zim::writer::Hints();
      }
    }

    virtual std::unique_ptr<zim::writer::ContentProvider> getContentProvider() const {
      return std::unique_ptr<zim::writer::ContentProvider>(new zim::writer::StringProvider(content));
    }

  std::string path;
  std::string title;
  std::string content;
  std::string mimetype;
  IsFrontArticle frontArticle;
};

} // namespace unittests

} // namespace zim

#endif // ZIM_TEST_TOOLS_H
