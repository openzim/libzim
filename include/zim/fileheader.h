/*
 * Copyright (C) 2008 Tommi Maekitalo
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

#ifndef ZIM_FILEHEADER_H
#define ZIM_FILEHEADER_H

#include <zim/zim.h>
#include <zim/endian.h>
#include <iosfwd>

namespace zim
{
  class Fileheader
  {
      friend std::ostream& operator<< (std::ostream& out, const Fileheader& fh);
      friend std::istream& operator>> (std::istream& in, Fileheader& fh);

    public:
      static const size_type magic;
      static const size_type version;
      static const unsigned headerSize = 60;
      static const unsigned headerFill = 58;

    private:
      char header[headerSize];

    public:
      Fileheader();

      size_type getMagicNumber() const   { return fromLittleEndian<size_type>(header + 0x0); }
      size_type getVersion() const       { return fromLittleEndian<size_type>(header + 0x4); }

      size_type getCount() const         { return fromLittleEndian<size_type>(header + 0x8); }
      void      setCount(size_type s)    { *reinterpret_cast<size_type*>(header + 8) = fromLittleEndian<size_type>(&s); }

      offset_type getIndexPos() const    { return fromLittleEndian<offset_type>(header + 0x10); }
      void        setIndexPos(offset_type p) { *reinterpret_cast<offset_type*>(header + 0x10) = fromLittleEndian<offset_type>(&p); }

      size_type getIndexLen() const      { return fromLittleEndian<size_type>(header + 0x18); }
      void      setIndexLen(size_type l) { *reinterpret_cast<size_type*>(header + 0x18) = fromLittleEndian<size_type>(&l); }

      offset_type getIndexPtrPos() const { return fromLittleEndian<offset_type>(header + 0x20); }
      void        setIndexPtrPos(offset_type p) { *reinterpret_cast<offset_type*>(header + 0x20) = fromLittleEndian<offset_type>(&p); }

      size_type getIndexPtrLen() const   { return fromLittleEndian<size_type>(header + 0x28); }
      void      setIndexPtrLen(size_type l) { *reinterpret_cast<size_type*>(header + 0x28) = fromLittleEndian<size_type>(&l); }

      offset_type getDataPos() const   { return getIndexPos() + getIndexLen(); }
  };

  std::ostream& operator<< (std::ostream& out, const Fileheader& fh);
  std::istream& operator>> (std::istream& in, Fileheader& fh);

}

#endif // ZIM_FILEHEADER_H
