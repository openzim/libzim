/*
 * Copyright (C) 2017 Matthieu Gautier <mgautier@kymeria.fr>
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


#include "zim/blob.h"
#include "debug.h"
#include "buffer.h"

namespace zim {

namespace
{

struct NoDelete
{
  template<class T> void operator()(T*) {}
};

// This shared_ptr is used as a source object for the std::shared_ptr
// aliasing constructor (with the purpose of avoiding the control block
// allocation) for the case when the referred data must not be deleted.
static Blob::DataPtr nonOwnedDataPtr((char*)nullptr, NoDelete());

} // unnamed namespace


Blob::Blob()
 : _data(nonOwnedDataPtr),
   _size(0)
{}

Blob::Blob(const char* data, size_type size)
 : _data(nonOwnedDataPtr, data),
   _size(size)
{
  ASSERT(size, <, SIZE_MAX);
  ASSERT(data, <, (void*)(SIZE_MAX-size));
}

Blob::Blob(const DataPtr& buffer, size_type size)
 : _data(buffer),
   _size(size)
{}




} //zim
