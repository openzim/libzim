/*
 * Copyright (C) 2026 Veloman Yunkan
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

#ifndef ZIM_WRITER_BINARY_FILE_H
#define ZIM_WRITER_BINARY_FILE_H

#include "config.h"
#include "zim/zim.h"

#include <string>
#include <stdio.h>

namespace zim
{

namespace writer
{

class LIBZIM_PRIVATE_API BinaryFile
{
public: // functions
  BinaryFile(const BinaryFile& ) = delete;
  void operator=(const BinaryFile& ) = delete;

  explicit BinaryFile(int fd = -1);
  ~BinaryFile();

  void openFile(const std::string& filePath);
  void closeFile();

  offset_type tellFilePos() const;
  void seek(offset_type pos);
  offset_type seekEnd();

  void write(const char* buf, size_t size);

  void flush();

private: // data
  // For our use cases C stdio proves to be significantly more efficient than
  // std::ofstream or (untuned) custom buffering over raw syscalls to write()
  FILE* file = nullptr;
};

} // namespace writer

} // namespace zim

#endif // ZIM_WRITER_BINARY_FILE_H
