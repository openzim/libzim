/*
 * Copyright 2013-2016 Emmanuel Engelhart <kelson@kiwix.org>
 * Copyright 2016 Matthieu Gautier <mgautier@kymeria.fr>
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

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <random>
#include <errno.h>

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

#if defined(ENABLE_XAPIAN)

#include <unicode/translit.h>
#include <unicode/ucnv.h>
std::string zim::removeAccents(const std::string& text)
{
  ucnv_setDefaultName("UTF-8");
  static UErrorCode status = U_ZERO_ERROR;
  static std::unique_ptr<icu::Transliterator> removeAccentsTrans(icu::Transliterator::createInstance(
      "Lower; NFD; [:M:] remove; NFC", UTRANS_FORWARD, status));
  icu::UnicodeString ustring(text.c_str());
  removeAccentsTrans->transliterate(ustring);
  std::string unaccentedText;
  ustring.toUTF8String(unaccentedText);
  return unaccentedText;
}
#endif

uint32_t zim::countWords(const std::string& text)
{
  unsigned int numWords = 1;
  unsigned int length = text.size();

  for (unsigned int i = 0; i < length;) {
    while (i < length && text[i] != ' ') {
      i++;
    }
    numWords++;
      i++;
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

uint32_t zim::randomNumber(uint32_t max)
{
  static std::default_random_engine random(
    std::chrono::system_clock::now().time_since_epoch().count());
  static std::mutex mutex;

  std::lock_guard<std::mutex> l(mutex);
  return ((double)random() / random.max()) * max;
}
