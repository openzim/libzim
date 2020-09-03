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

#ifndef ZIM_READERDATASTREAMWRAPPER_H
#define ZIM_READERDATASTREAMWRAPPER_H

#include "idatastream.h"
#include "reader.h"
#include "debug.h"

namespace zim
{

class ReaderDataStreamWrapper : public IDataStream
{
public: // functions
  explicit ReaderDataStreamWrapper(std::shared_ptr<const zim::Reader> reader)
    : reader_(reader)
    , readerPos_(0)
  {}

  void readImpl(void* buf, size_t nbytes) override
  {
    ASSERT(readerPos_.v + nbytes, <=, reader_->size().v);
    reader_->read(static_cast<char*>(buf), readerPos_, zsize_t(nbytes));
    readerPos_ += nbytes;
  }

private: // data
  std::shared_ptr<const Reader> reader_;
  offset_t readerPos_;
};

} // namespace zim

#endif // ZIM_READERDATASTREAMWRAPPER_H
