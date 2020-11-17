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
#include <sys/types.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include "../src/buffer.h"

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
zim::Buffer write_to_buffer(const T& object)
{
  TempFile tmpFile("test_temp_file");
  const auto tmp_fd = tmpFile.fd();
  object.write(tmp_fd);
  auto size = lseek(tmp_fd, 0, SEEK_END);

  auto buf = zim::Buffer::makeBuffer(zim::zsize_t(size));
  lseek(tmp_fd, 0, SEEK_SET);
  if (read(tmp_fd, const_cast<char*>(buf.data()), size) == -1)
    throw std::runtime_error("Cannot read");
  return buf;
}

} // namespace unittests

} // namespace zim

#endif // ZIM_TEST_TOOLS_H
