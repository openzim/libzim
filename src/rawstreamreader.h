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

#ifndef ZIM_RAWSTREAMREADER_H
#define ZIM_RAWSTREAMREADER_H

#include "istreamreader.h"
#include "reader.h"
#include "debug.h"

namespace zim
{

class RawStreamReader : public IStreamReader
{
public: // functions
  explicit RawStreamReader(std::shared_ptr<const zim::Reader> reader)
    : m_reader(reader),
      m_readerPos(0)
  {}

  void readImpl(char* buf, zsize_t nbytes) override
  {
    m_reader->read(static_cast<char*>(buf), m_readerPos, zsize_t(nbytes));
    m_readerPos += nbytes;
  }

  std::unique_ptr<const Reader> sub_reader(zsize_t nbytes) override
  {
    auto reader = m_reader->sub_reader(m_readerPos, nbytes);
    m_readerPos += nbytes;
    return reader;
  }


private: // data
  std::shared_ptr<const Reader> m_reader;
  offset_t m_readerPos;
};

} // namespace zim

#endif // ZIM_READERDATASTREAMWRAPPER_H
