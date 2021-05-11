#!/usr/bin/env python3

'''
Copyright 2021 Matthieu Gautier <mgautier@kymeria.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or any
later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.
'''

import argparse
from pathlib import Path
from urllib import request
from urllib.error import *
import tarfile
import sys

TEST_DATA_VERSION = "0.3"
ARCHIVE_URL_TEMPL = "https://github.com/openzim/zim-testing-suite/releases/download/v{version}-alpha/zim-testing-suite-{version}.tar.gz"

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--version', '-v',
                        help="The version to download.",
                        default=TEST_DATA_VERSION)
    parser.add_argument('--remove-top-dir',
                        help="Remove the top directory when extracting",
                        action='store_true')
    parser.add_argument('outdir',
                        help='The directory where to install the test data.')
    args = parser.parse_args()

    test_data_url = ARCHIVE_URL_TEMPL.format(version=args.version)

    try:
        with request.urlopen(test_data_url) as f:
            with tarfile.open(fileobj=f, mode="r|*") as archive:
                while True:
                    member = archive.next()
                    if member is None:
                        break
                    if args.remove_top_dir:
                        member.name = '/'.join(member.name.split('/')[1:])
                    archive.extract(member, path=args.outdir)

    except HTTPError as e:
        print("Error downloading archive at url : {}".format(test_data_url))
        print(e)
        sys.exit(1)
    except OSError as e:
        print("Error writing the test data on the file system.")
        print(e)
        sys.exit(1)
