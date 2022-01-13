/*
 * Copyright (C) 2020-2021 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2020 Emmanuel Engelhart <kelson@kiwix.org>
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

#ifndef _LIBZIM_COMPRESSION_LEVELS_
#define _LIBZIM_COMPRESSION_LEVELS_

enum class LZMACompressionLevel: int {
  MINIMUM = 0,
  DEFAULT = 5,
  MAXIMUM = 9
};

enum class ZSTDCompressionLevel: int {
  MINIMUM = -21,
  DEFAULT = 3,
  MAXIMUM = 21
};

#endif // _LIBZIM_COMPRESSION_LEVELS_
