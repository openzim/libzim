/*
 * Copyright (C) 2021 Maneesh P M <manu.pm55@gmail.com>
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

#define ANCHOR_TERM "0posanchor "

#define DEFAULT_CLUSTER_SIZE 2*1024*1024

// The size in bytes of the longest word that is indexable in a title.
// Xapian's default value is 64 while the hard limit is 245, however crashes
// have been observed with values as low as 150 (demonstrated by the unit test
// Suggestion.handlingOfTooLongWords in test/suggestion.cpp).
// Note that a similar limit applies to full-text indexing but we don't
// provide a way to control it (so it is at Xapian's default value of 64)
#define MAX_INDEXABLE_TITLE_WORD_SIZE 64
