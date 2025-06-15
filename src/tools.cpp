/*
 * Copyright (C) 2016-2021 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2021 Maneeshs P M <manu.pm55@gmail.com>
 * Copyright (C) 2013-2016 Emmanuel Engelhart <kelson@kiwix.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU  General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "tools.h"
#include "zim/tools.h"
#include "zim/illustration.h"
#include "fs.h"
#include "writer/_dirent.h"

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mutex>
#include <stdexcept>
#include <random>
#include <sstream>

#ifdef _WIN32
# include <windows.h>
# include <direct.h>
# include <io.h>
# include <stringapiset.h>
# define SEPARATOR "\\"
#else
# include <unistd.h>
# define SEPARATOR "/"
#endif

#ifdef __MINGW32__
# include <time.h>
#else
# include <thread>
# include <chrono>
#endif

bool zim::isCompressibleMimetype(const std::string& mimetype)
{
  return mimetype.find("text") == 0
      || mimetype.find("+xml") != std::string::npos
      || mimetype.find("+json") != std::string::npos
      || mimetype == "application/javascript"
      || mimetype == "application/json";
}

uint32_t zim::countWords(const std::string& text)
{
  unsigned int numWords = 0;
  unsigned int length = text.size();
  unsigned int i = 0;

  // Find first word
  while ( i < length && std::isspace(static_cast<unsigned char>(text[i])) ) i++;

  while ( i < length ) {
    // Find end of word
    while ( i < length && !std::isspace(static_cast<unsigned char>(text[i])) ) i++;
    numWords++;
    // Find start of next word
    while ( i < length && std::isspace(static_cast<unsigned char>(text[i])) ) i++;
  }
  return numWords;
}


void zim::microsleep(int microseconds) {
#ifdef __MINGW32__
   struct timespec wait = {0, 0};
   wait.tv_sec = microseconds / 1000000;
   wait.tv_nsec = (microseconds - wait.tv_sec*10000) * 1000;
   nanosleep(&wait, nullptr);
#else
   std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
#endif
}


std::tuple<char, std::string> zim::parseLongPath(const std::string& longPath)
{
  /* Index of the namespace char; discard '/' from absolute paths */
  const unsigned int i = (longPath[0] == '/') ? 1 : 0;
  if (i + 1 > longPath.size() || longPath[i] == '/' || (i + 1 < longPath.size() && longPath[i+1] != '/'))
    throw std::runtime_error("Cannot parse path");

  auto ns = longPath[i];
  auto shortPath = longPath.substr(std::min<unsigned int>(i+2, (unsigned int)longPath.size()));

  return std::make_tuple(ns, shortPath);
}

unsigned int zim::parseIllustrationPathToSize(const std::string& s)
{
  int nw(0), nh(0), nEnd(0);
  long int w(-1), h(-1);
  if ( sscanf(s.c_str(), "Illustration_%n%ldx%n%ld@1%n", &nw, &w, &nh, &h, &nEnd) == 2
     && (size_t)nEnd == s.size() && !isspace(s[nw]) && !isspace(s[nh]) && w == h && w >= 0) {
    return (unsigned int)w;
  }
  throw std::runtime_error("");
}

std::string zim::IllustrationInfo::asMetadataItemName() const
{
  std::ostringstream oss;
  oss << "Illustration_" << width << "x" << height << "@" << scale;
  for ( const auto& kv : extraAttributes ) {
    oss << ";" << kv.first << "=" << kv.second;
  }
  return oss.str();
}

zim::IllustrationInfo zim::IllustrationInfo::fromMetadataItemName(const std::string& s)
{
  int nw(0), nh(0), ns(0), nEnd(0);
  long int w(-1), h(-1);
  float scale(0);

  const char fmt[] = "Illustration_%n%ldx%n%ld@%n%f%n";
  if ( sscanf(s.c_str(), fmt, &nw, &w, &nh, &h, &ns, &scale, &nEnd) == 3
     && (size_t)nEnd == s.size()
     && !isspace(s[nw])
     && !isspace(s[nh])
     && !isspace(s[ns])
     && w >= 0
     && h >= 0
     && scale >= 0 ) {
    return IllustrationInfo{uint32_t(w), uint32_t(h), scale, {}};
  }
  throw std::runtime_error("");
}

uint32_t zim::randomNumber(uint32_t max)
{
  static std::default_random_engine random(
    std::chrono::system_clock::now().time_since_epoch().count());
  static std::mutex mutex;

  std::lock_guard<std::mutex> l(mutex);
  return (uint32_t)(((double)random() / random.max()) * max);
}

/* Split string in a token array */
std::vector<std::string> zim::split(const std::string & str,
                                const std::string & delims)
{
  std::string::size_type lastPos = str.find_first_not_of(delims, 0);
  std::string::size_type pos = str.find_first_of(delims, lastPos);
  std::vector<std::string> tokens;

  while (std::string::npos != pos || std::string::npos != lastPos)
    {
      tokens.push_back(str.substr(lastPos, pos - lastPos));
      lastPos = str.find_first_not_of(delims, pos);
      pos     = str.find_first_of(delims, lastPos);
    }

  return tokens;
}

std::map<std::string, int> zim::read_valuesmap(const std::string &s) {
    std::map<std::string, int> result;
    std::vector<std::string> elems = split(s, ";");
    for(std::vector<std::string>::iterator elem = elems.begin();
        elem != elems.end();
        elem++)
    {
        std::vector<std::string> tmp_elems = split(*elem, ":");
        result.insert( std::pair<std::string, int>(tmp_elems[0], atoi(tmp_elems[1].c_str())) );
    }
    return result;
}

namespace
{
// The counter metadata format is a list of item separated by a `;` :
// item0;item1;item2
// Each item is a "tuple" mimetype=number.
// However, the mimetype may contains parameters:
// text/html;raw=true;foo=bar
// So the final format may be complex to parse:
// key0=value0;key1;foo=bar=value1;key2=value2

typedef zim::MimeCounterType::value_type MimetypeAndCounter;

std::string readFullMimetypeAndCounterString(std::istream& in)
{
  std::string mtcStr, params;
  getline(in, mtcStr, ';');
  if ( mtcStr.find('=') == std::string::npos )
  {
    do
    {
      if ( !getline(in, params, ';' ) )
        return std::string();
      mtcStr += ";" + params;
    }
    while ( std::count(params.begin(), params.end(), '=') != 2 );
  }
  return mtcStr;
}

MimetypeAndCounter parseASingleMimetypeCounter(const std::string& s)
{
  const std::string::size_type k = s.find_last_of("=");
  if ( k != std::string::npos )
  {
    const std::string mimeType = s.substr(0, k);
    std::istringstream counterSS(s.substr(k+1));
    unsigned int counter;
    if (counterSS >> counter && counterSS.eof())
      return std::make_pair(mimeType, counter);
  }
  return MimetypeAndCounter{"", 0};
}

} // unnamed namespace

zim::MimeCounterType zim::parseMimetypeCounter(const std::string& counterData)
{
  zim::MimeCounterType counters;
  std::istringstream ss(counterData);

  while (ss)
  {
    const std::string mtcStr = readFullMimetypeAndCounterString(ss);
    const MimetypeAndCounter mtc = parseASingleMimetypeCounter(mtcStr);
    if ( !mtc.first.empty() )
      counters.insert(mtc);
  }

  return counters;
}

bool zim::writer::isFrontArticle(const zim::writer::Dirent* dirent, const zim::writer::Hints& hints)
{
  // By definition, dirent not in `C` namespace are not FRONT_ARTICLE
  if (dirent->getNamespace() != NS::C) {
    return false;
  }
  try {
    return bool(hints.at(FRONT_ARTICLE));
  } catch(std::out_of_range&) {
    return false;
  }
}


// Xapian based tools
#if defined(ENABLE_XAPIAN)

#include "xapian.h"

#include <unicode/translit.h>
#include <unicode/ucnv.h>
#include <unicode/putil.h>

#define BATCH_SIZE (4*1024)
std::string zim::removeAccents(const std::string& text)
{
  ucnv_setDefaultName("UTF-8");
  static UErrorCode status = U_ZERO_ERROR;
  static std::unique_ptr<icu::Transliterator> removeAccentsTrans(icu::Transliterator::createInstance(
      "Lower; NFD; [:M:] remove; NFC", UTRANS_FORWARD, status));
  icu::UnicodeString ustring(text.c_str());
  std::string unaccentedText;

  auto nb_chars = ustring.length();
  if (nb_chars <= BATCH_SIZE) {
    // Remove accents in one step.
    removeAccentsTrans->transliterate(ustring);
    ustring.toUTF8String(unaccentedText);
  } else {
    auto current_pos = 0;
    icu::UnicodeString current_ustring;
    while (current_pos < nb_chars) {
      // Remove accents by batch of BATCH_SIZE "chars" to avoid working on
      // a too long string and spending to much time memcpy things.
      auto end = ustring.getChar32Limit(current_pos+BATCH_SIZE);
      auto current_size = end - current_pos;
      current_ustring.remove();
      ustring.extract(current_pos, current_size, current_ustring);
      removeAccentsTrans->transliterate(current_ustring);
      current_ustring.toUTF8String(unaccentedText);
      current_pos += current_size;
    }
  }
  return unaccentedText;
}

bool zim::getDbFromAccessInfo(zim::ItemDataDirectAccessInfo accessInfo, Xapian::Database& database) {
  zim::DEFAULTFS::FD databasefd;
  try {
      databasefd = zim::DEFAULTFS::openFile(accessInfo.filename);
  } catch (...) {
      std::cerr << "Impossible to open " << accessInfo.filename << std::endl;
      std::cerr << strerror(errno) << std::endl;
      return false;
  }
  if (!databasefd.seek(zim::offset_t(accessInfo.offset))) {
      std::cerr << "Something went wrong seeking databasedb "
                << accessInfo.filename << std::endl;
      std::cerr << "dbOffest = " << accessInfo.offset << std::endl;
      return false;
  }

  try {
      database = Xapian::Database(databasefd.release());
  } catch( Xapian::DatabaseError& e) {
      std::cerr << "Something went wrong opening xapian database for zimfile "
                << accessInfo.filename << std::endl;
      std::cerr << "dbOffest = " << accessInfo.offset << std::endl;
      std::cerr << "error = " << e.get_msg() << std::endl;
      return false;
  }

  return true;
}

void zim::setICUDataDirectory(const std::string& path)
{
  u_setDataDirectory(path.c_str());
}
#endif
