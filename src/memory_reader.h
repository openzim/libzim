/*
 * Copyright (C) 2017 Matthieu Gautier <mgautier@kymeria.fr>
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

#ifndef ZIM_MEMORY_READER_H_
#define ZIM_MEMORY_READER_H_

#include "reader.h"

namespace zim
{

class MemoryReader : public Reader
{
public:
  explicit MemoryReader(Blob data) : data_(data) {}
  virtual ~MemoryReader() {}

  zsize_t size() const override;
  offset_t offset() const;

  void read(char* dest, offset_t offset, zsize_t size) const override;
  char read(offset_t offset) const override;
  Blob read_blob(offset_t offset, zsize_t size) const override;
  std::unique_ptr<const Reader> sub_reader(offset_t offset, zsize_t size) const override;

private:
  const Blob data_;
};

} // namespace zim

#endif // ZIM_MEMORY_READER_H_
