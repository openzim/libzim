/*
 * Copyright (C) 2009 Tommi Maekitalo
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

#include "config.h"

#include "zimcreatordata.h"
#include "cluster.h"
#include "debug.h"
#include <zim/blob.h>
#include "../endian_tools.h"
#include <algorithm>
#include <fstream>

#if defined(ENABLE_XAPIAN)
  #include "xapianIndexer.h"
#endif

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <limits>
#include <stdexcept>
#include <sstream>
#include "md5stream.h"
#include "tee.h"
#include "log.h"
#include "../fs.h"
#include "../tools.h"

namespace zim
{
  namespace writer
  {
    void* clusterWriter(void* arg) {
      auto zimCreatorData = static_cast<zim::writer::ZimCreatorData*>(arg);
      zim::writer::Cluster* clusterToWrite;
      unsigned int wait = 0;

      while(true) {
        microsleep(wait);
        if (zimCreatorData->clustersToWrite.popFromQueue(clusterToWrite)) {
          wait = 0;
          clusterToWrite->dump_tmp(zimCreatorData->tmpfname);
          clusterToWrite->close();
          continue;
        }
        wait += 10;
      }
      return nullptr;
    }
  }
}
