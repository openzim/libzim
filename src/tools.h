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

#ifndef OPENZIM_LIBZIM_TOOLS_H
#define OPENZIM_LIBZIM_TOOLS_H

#include <string>

namespace zim {

  std::string removeAccents(const std::string& text);

  /**
   * Open a file in BINARY and READONLY mode and return
   * the corresponding fd.
   *
   * @param filepath An utf8 encoded path.
   * @return the file descriptor of the open file.
   * @error If the file cannot be opened, return -1 and set errno.
   */
  int  openFile(const std::string& filepath);
  bool makeDirectory(const std::string& path);
  void remove_all(const std::string& path);
  void move(const std::string& old_path,
            const std::string& new_path);

}

#endif  // OPENZIM_LIBZIM_TOOLS_H
