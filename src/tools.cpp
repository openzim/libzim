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
#include <unistd.h>
#include <sys/stat.h>
#include <memory>

#include <unicode/translit.h>
#include <unicode/ucnv.h>

#ifdef _WIN32
#include <direct.h>
#define SEPARATOR "\\"
#else
#define SEPARATOR "/"
#endif


std::string zim::removeAccents(const std::string& text)
{
  ucnv_setDefaultName("UTF-8");
  static UErrorCode status = U_ZERO_ERROR;
  static std::unique_ptr<Transliterator> removeAccentsTrans(Transliterator::createInstance(
      "Lower; NFD; [:M:] remove; NFC", UTRANS_FORWARD, status));
  UnicodeString ustring = UnicodeString(text.c_str());
  removeAccentsTrans->transliterate(ustring);
  std::string unaccentedText;
  ustring.toUTF8String(unaccentedText);
  return unaccentedText;
}

void zim::remove_all(const std::string& path)
{
  DIR* dir;

  /* It's a directory, remove all its entries first */
  if ((dir = opendir(path.c_str())) != NULL) {
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
      if (strcmp(ent->d_name, ".") and strcmp(ent->d_name, "..")) {
        std::string childPath = path + SEPARATOR + ent->d_name;
        remove_all(childPath);
      }
    }
    closedir(dir);
    rmdir(path.c_str());
  }

  /* It's a file */
  else {
    remove(path.c_str());
  }
}

bool zim::makeDirectory(const std::string& path)
{
#ifdef _WIN32
  int status = _mkdir(path.c_str());
#else
  int status = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
  return status == 0;
}
