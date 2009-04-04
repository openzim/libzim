/*
 * Copyright (C) 2006 Tommi Maekitalo
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

#include <zim/qunicode.h>
#include <cxxtools/log.h>
#include <iostream>
#include <sstream>

log_define("zim.qunicode")

namespace zim
{
  namespace
  {
    static const char qunicode[] = "\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037\037 !\"#$%&'()*+,-./0123456789:;<=>\?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\134]^_`ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}~\177E\201\202F\204\205\206\207\210\211S\213O\215Z\217\220\221\222\223\224\225\226\227\230\231S\233O\235ZY\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277AAAAAAACEEEEIIIIDNOOOOO\327OUUUUYTSAAAAAAACEEEEIIIIDNOOOOO\367OUUUUYTYAAAAAACCCCCCCCDDDDEEEEEEEEEEGGGGGGGGHHHHIIIIIIIIIIIIJJKKKLLLLLLLLLLNNNNNNNNNOOOOOOOORRRRRRSSSSSSSSTTTTTTUUUUUUUUUUUUWWYYYZZZZZZSBBBBBBCCCDDDDDEEEFFGGHIIKKLLMNNOOOOOPPRSSSSTTTTUUUVYYZZZZZZZZZZZ----DDDLLLNNNAAIIOOUUUUUUUUUUEAAAAAAGGGGKKOOOOZZJDDDGGHHNNAAAAOOAAAAEEEEIIIIOOOORRRRUUUUSSTTZZHH----ZZAAEEOOOOOOOOYY----------------------------AAABCCDDEEEEEEEFGGGGGHHHIIIIIIIMMMNNNOOOORRRRRSSRRSSSSSTTUUUVWYYZZZZ-----BGGHJKLD--DDDTTTFLLWU---------WY-------AABBBBBBCCDDDDDDDDDDEEEEEEEEEEFFGGHHHHHHHHHHIIIIKKKKKKLLLLLLLLMMMMMMNNNNNNNNOOOOOOOOPPPPRRRRRRRRSSSSSSSSSSTTTTTTTTUUUUUUUUUUVVVVWWWWWWWWWWXXXXYYZZZZZZHTWYA---SS";

    void appendHex4(std::string& s, uint16_t v)
    {
      static const char* hex = "0123456789abcdef";
      s += hex[(v >> 12) & 0x0f];
      s += hex[(v >> 8) & 0x0f];
      s += hex[(v >> 4) & 0x0f];
      s += hex[0x0f];
    }
  }

  QUnicodeString QUnicodeString::fromUtf8(const std::string& v)
  {
    QUnicodeString ret;

    unsigned bytes = 0;
    uint16_t uvalue = 0;
    for (std::string::const_iterator it = v.begin(); it != v.end(); ++it)
    {
      unsigned char ch = static_cast<unsigned char>(*it);
      if (bytes)
      {
        uvalue = ((uvalue << 6) | ch & 0x3f);
        if (--bytes == 0)
        {
          if (uvalue <= 0xff)
          {
            // unicode value fits one byte; no qunicode escape character
            ret.value += static_cast<char>(uvalue);
          }
          else if (uvalue & 0xff)
          {
            // lower byte value is not 0 - use qunicode escape 1
            ret.value += '\1';
            ret.value += static_cast<char>(uvalue & 0xff);
            ret.value += static_cast<char>(uvalue >> 8);
          }
          else
          {
            // lower byte value is 0 - use qunicode escape 2 and replace lower byte with 1
            ret.value += '\2';
            ret.value += '\1';
            ret.value += static_cast<char>(uvalue >> 8);
          }
        }
      }
      else if (ch >= 0xf0)
      {
        bytes = 3;
        uvalue = ch & 0x07;
      }
      else if (ch >= 0xe0)
      {
        bytes = 2;
        uvalue = ch & 0x0f;
      }
      else if (ch >= 0xc0)
      {
        bytes = 1;
        uvalue = ch & 0x1f;
      }
      else
        ret.value += *it;
    }

    return ret;
  }

  unsigned char QUnicodeChar::getCollateValue() const
  {
    return value < sizeof(qunicode) ? qunicode[value] : '\x2d';
  }

  std::istream& operator>> (std::istream& in, QUnicodeChar& qc)
  {
    char ch;
    in.get(ch);
    if (in)
    {
      switch (ch)
      {
        case '\1':
        {
          char lo, hi;
          in.get(lo);
          in.get(hi);
          if (in)
            qc = QUnicodeChar(hi, lo);
          break;
        }
        case '\2':
        {
          char lo, hi;
          in.get(lo);
          in.get(hi);
          if (in)
            qc = QUnicodeChar(hi, '\0');
          break;
        }
        default:
          qc = QUnicodeChar(ch);
      }
    }
    return in;
  }

  std::string QUnicodeString::toXML() const
  {
    std::ostringstream ret;
    for (std::string::size_type n = 0; n < value.size(); ++n)
    {
      switch(value[n])
      {
        case '\1':
        {
          unsigned char lo = value[++n];
          unsigned char hi = value[++n];
          uint16_t uvalue = static_cast<uint16_t>(hi) << 8 | static_cast<uint16_t>(lo);
          ret << "&#" << uvalue << ';';
          break;
        }

        case '\2':
        {
          unsigned char hi = value[++n];
          ++n;
          uint16_t uvalue = static_cast<uint16_t>(hi) << 8;
          ret << "&#" << uvalue << ';';
          break;
        }

        case '<':
          ret << "&lt;";
          break;

        case '>':
          ret << "&gt;";
          break;

        case '&':
          ret << "&et;";
          break;

        case '"':
          ret << "&quot;";
          break;

        case '\'':
          ret << "&#39;";
          break;

        default:
          if (value[n] >= 0)
            ret << value[n];
          else
          {
            ret << static_cast<char>(('\xc0' | (static_cast<unsigned char>(value[n]) >> 6)))
                << static_cast<char>(('\x80' | value[n] & '\x3f'));
          }
      }
    }
    return ret.str();
  }

  std::string QUnicodeString::toUtf8() const
  {
    std::string ret;
    for (std::string::size_type n = 0; n < value.size(); ++n)
    {
      switch(value[n])
      {
        case '\1':
        {
          unsigned char lo = value[++n];
          unsigned char hi = value[++n];
          uint16_t uvalue = static_cast<uint16_t>(hi) << 8 | static_cast<uint16_t>(lo);

          if (uvalue < 128)
            ret += static_cast<char>(uvalue);
          else if (uvalue < 2048)
          {
            ret += ('\xc0' | (uvalue >> 6));
            ret += ('\x80' | (uvalue & 0x3f));
          }
          else
          {
            ret += ('\xe0' | (uvalue >> 12));
            ret += ('\x80' | ((uvalue >> 6) & 0x3f));
            ret += ('\x80' | (uvalue & 0x3f));
          }
          break;
        }

        case '\2':
        {
          unsigned char hi = value[++n];
          ++n;
          uint16_t uvalue = static_cast<uint16_t>(hi) << 8;

          if (uvalue < 128)
            ret += static_cast<char>(uvalue);
          else if (uvalue < 2048)
          {
            ret += ('\xc0' | (uvalue >> 6));
            ret += ('\x80' | (uvalue & 0x3f));
          }
          else
          {
            ret += ('\xe0' | (uvalue >> 12));
            ret += ('\x80' | ((uvalue >> 6) & 0x3f));
            ret += ('\x80' | (uvalue & 0x3f));
          }
          break;
        }

        default:
          if (value[n] >= 0)
            ret += value[n];
          else
          {
            ret += ('\xc0' | (static_cast<unsigned char>(value[n]) >> 6));
            ret += ('\x80' | value[n] & '\x3f');
          }
      }
    }
    return ret;
  }

  int QUnicodeString::compare(unsigned pos, unsigned n, const QUnicodeString& v) const
  {
    log_debug('"' << *this << "\".compare(" << pos << ", " << n << ", \"" << v << "\")");
    int coll = compareCollate(pos, n, v);
    log_debug("compare collate returns " << coll);
    if (coll != 0)
      return coll;

    std::istringstream is1(value);
    std::istringstream is2(v.value);

    unsigned p;
    QUnicodeChar uc1, uc2;

    for (p = 0; p < pos + n; ++p)
    {
      is1 >> uc1;
      is2 >> uc2;

      if (p >= pos)
      {
        if (!is1)
          return is2 ? -1 : 0;
        else if (!is2)
          return 1;

        if (uc1.getValue() < uc2.getValue())
          return -1;
        else if (uc2.getValue() < uc1.getValue())
          return 1;
      }
    }

    is2.get();
    return is2 ? -1 : 0;
  }

  int QUnicodeString::compareCollate(unsigned pos, unsigned n, const QUnicodeString& v) const
  {
    log_debug('"' << *this << "\".compareCollate(" << pos << ", " << n << ", \"" << v << "\")");

    std::istringstream is1(value);
    std::istringstream is2(v.value);

    unsigned p;
    QUnicodeChar uc1, uc2;

    for (p = 0; p < pos + n; ++p)
    {
      is1 >> uc1;
      is2 >> uc2;

      if (p >= pos)
      {
        if (!is1)
          return is2 ? -1 : 0;
        else if (!is2)
          return 1;

        if (uc1.getCollateValue() < uc2.getCollateValue())
          return -1;
        else if (uc2.getCollateValue() < uc1.getCollateValue())
          return 1;
      }
    }

    is2.get();
    return is2 ? -1 : 0;
  }

  unsigned QUnicodeString::size() const
  {
    std::istringstream is(value);
    QUnicodeChar uc;
    unsigned ret = 0;
    while (is >> uc)
      ++ret;
    return ret;
  }

  std::string QUnicodeString::getCollateString() const
  {
    std::istringstream is(value);
    std::string ret;
    QUnicodeChar uc;
    while (is >> uc)
      ret += static_cast<char>(uc.getCollateValue());
    return ret;
  }

  std::ostream& operator<< (std::ostream& out, const QUnicodeString& str)
  {
    return out << str.toUtf8();
  }

}
