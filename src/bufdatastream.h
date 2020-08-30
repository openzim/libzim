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

#ifndef ZIM_BUFDATASTREAM_H
#define ZIM_BUFDATASTREAM_H

#include "idatastream.h"
#include "debug.h"

#include <string.h>

namespace zim
{

class BufDataStream : public IDataStream
{
public: // functions
  explicit BufDataStream(const char* data, size_t size)
    : data_(data)
    , size_(size)
  {}

protected:
  void readImpl(void* buf, size_t nbytes) override;
  Blob readBlobImpl(size_t size) override;


private: // data
  const char* data_;
  size_t size_;
};

} // namespace zim

#endif // ZIM_BUFDATASTREAM_H
