/*
 * Copyright (C) 2003 Tommi Maekitalo
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * As a special exception, you may use this file as part of a free
 * software library without restriction. Specifically, if other files
 * instantiate templates or use macros or inline functions from this
 * file, or you compile this file and link it with other files to
 * produce an executable, this file does not by itself cause the
 * resulting executable to be covered by the GNU General Public
 * License. This exception does not however invalidate any other
 * reasons why the executable file might be covered by the GNU Library
 * General Public License.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef ZIM_TEE_H
#define ZIM_TEE_H

#include <iostream>

namespace zim
{

class Teestreambuf : public std::streambuf
{
  public:
    Teestreambuf(std::streambuf* buf1 = 0, std::streambuf* buf2 = 0)
      : streambuf1(buf1),
        streambuf2(buf2)
    { setp(0, 0); }
    
    void tie(std::streambuf* buf1, std::streambuf* buf2 = 0)
    {
      streambuf1 = buf1;
      streambuf2 = buf2;
    }

  private:
    std::streambuf::int_type overflow(std::streambuf::int_type ch);
    std::streambuf::int_type underflow();
    int sync();

    std::streambuf* streambuf1;
    std::streambuf* streambuf2;
};

/////////////////////////////////////////////////////////////////////////////

class Tee : public std::ostream
{
    typedef std::ostream base_class;
    Teestreambuf streambuf;

  public:
    Tee()
      : std::ostream(0),
        streambuf(std::cout.rdbuf())
    {
      init(&streambuf);
    }
    Tee(std::ostream& s1, std::ostream& s2)
      : std::ostream(0),
        streambuf(s1.rdbuf(), s2.rdbuf())
    {
      init(&streambuf);
    }
    Tee(std::ostream& s)
      : std::ostream(0),
        streambuf(s.rdbuf(), std::cout.rdbuf())
    {
      init(&streambuf);
    }

    void assign(std::ostream& s1, std::ostream& s2);
    void assign(std::ostream& s)
    { assign(s, std::cout); }
    void assign_single(std::ostream& s);
};

}

#endif  // ZIM_TEE_H
