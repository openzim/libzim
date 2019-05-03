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
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory>
#include <errno.h>

#include <unicode/translit.h>
#include <unicode/ucnv.h>

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
