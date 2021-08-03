Libzim
======

The Libzim is the reference implementation for the [ZIM file
format](https://wiki.openzim.org/wiki/ZIM_file_format). It's a [software
library](https://en.wikipedia.org/wiki/Library_(computing)) to read
and write ZIM files on many systems and architectures. More
information about the ZIM format and the openZIM project at
https://openzim.org/.

[![Release](https://img.shields.io/github/v/tag/openzim/libzim?label=release&sort=semver)](https://download.openzim.org/release/libzim/)
[![Repositories](https://img.shields.io/repology/repositories/libzim?label=repositories)](https://github.com/openzim/libzim/wiki/Repology)
[![License](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Build](https://github.com/openzim/libzim/workflows/CI/badge.svg?query=branch%3Amaster)](https://github.com/openzim/libzim/actions?query=branch%3Amaster)
[![Doc](https://readthedocs.org/projects/libzim/badge/?style=flat)](https://libzim.readthedocs.io/en/latest/?badge=latest)
[![Codecov](https://codecov.io/gh/openzim/libzim/branch/master/graph/badge.svg)](https://codecov.io/gh/openzim/libzim)
[![CodeFactor](https://www.codefactor.io/repository/github/openzim/libzim/badge)](https://www.codefactor.io/repository/github/openzim/libzim)

Disclaimer
----------

This document assumes you have a little knowledge about software
compilation. If you experience difficulties with the dependencies or
with the Libzim compilation itself, we recommend to have a look to
[kiwix-build](https://github.com/kiwix/kiwix-build).

Preamble
--------

Although the Libzim can be compiled/cross-compiled on/for many
systems, the following documentation explains how to do it on POSIX
ones. It is primarily though for GNU/Linux systems and has been tested
on recent releases of Ubuntu and Fedora.

Dependencies
------------

The Libzim relies on many third party software libraries. They are
prerequisites to the Kiwix library compilation. Following libraries
need to be available:
* [LZMA](https://tukaani.org/lzma/) (package `liblzma-dev` on Ubuntu)
* [ICU](http://site.icu-project.org/) (package `libicu-dev` on Ubuntu)
* [Zstd](https://facebook.github.io/zstd/) (package `libzstd-dev` on Ubuntu)
* [Xapian](https://xapian.org/) - optional (package `libxapian-dev` on Ubuntu)
* [UUID](http://e2fsprogs.sourceforge.net/) (package `uuid-dev` on Ubuntu)

To test the code:
* [Google Test](https://github.com/google/googletest) (package `googletest` on Ubuntu)
* [ZIM Testing Suite](https://github.com/openzim/zim-testing-suite) - Reference test data set

To build the documentations you need the packages:
* [Doxygen](https://www.doxygen.nl)
* Python packages for [Sphinx](https://www.sphinx-doc.org), [Breathe](https://breathe.readthedocs.io) and [Exhale](https://exhale.readthedocs.io)

These dependencies may or may not be packaged by your operating
system. They may also be packaged but only in an older version. The
compilation script will tell you if one of them is missing or too old.
In the worse case, you will have to download and compile a more recent
version by hand.

If you want to install these dependencies locally, then ensure that
Meson (through `pkg-config`) will properly find them.

Environment
-------------

The Libzim builds using [Meson](https://mesonbuild.com/) version
0.43 or higher. Meson relies itself on Ninja, Pkg-config and few other
compilation tools. Install them first:
* Meson
* Ninja
* Pkg-config

These tools should be packaged if you use a cutting edge operating
system. If not, have a look to the [Troubleshooting](#Troubleshooting)
section.

Compilation
-----------

Once all dependencies are installed, you can compile Libzim with:
```bash
meson . build
ninja -C build
```

By default, it will compile dynamic linked libraries. All binary files
will be created in the `build` directory created automatically by
Meson. If you want statically linked libraries, you can add
`--default-library=static` option to the Meson command.

If you want to build the documentation, we need to pass the
`-Ddoc=true` option and run the `doc` target:
```bash
meson . build -Ddoc=true
ninja -C build doc
```

Depending on your system, `ninja` command may be called `ninja-build`.

By default, Libzim tries to compile with Xapian (and will generate an
error if Xapian is not found).  You can build without Xapian by
passing the option `-Dwith_xapian=false` :
```bash
meson . build -Dwith_xapian=false
ninja -C build doc
```

If Libzim is compiled without Xapian, all search API are removed.  You
can test if an installed version of Libzim is compiled with or without
xapian by testing the define `LIBZIM_WITH_XAPIAN`.

Testing
-------

ZIM files needed by unit-tests are not included in this repository. By
default, Meson will use an internal directory in your build directory,
but you can specify another directory with option `test_data_dir`:
```bash
meson . build -Dtest_data_dir=<A_DIR_WITH_TEST_DATA>
```

Whatever you specify a directory or not, you need a extra step to
download the data. At choice:
* Get the data from the repository
  [openzim/zim-testing-suite](https://github.com/openzim/zim-testing-suite)
  and put it yourself in the directory.
* Use the script
  [download_test_data.py](scripts/download_test_data.py) which will
  download and extract the data for you.
* As `ninja` to do it for you with `ninja download_test_data` once the
  project is configured.

The simple workflow is:
```bash
meson . build # Configure the project (using default directory for test data)
cd build
ninja # Build
ninja download_test_data # Download the test data
meson test # Test
```

It is possible to deactivate all tests using test data zim files by
passing `none` to the `test_data_dir` option:
```bash
meson . build -Dtest_data_dir=none
cd build
ninja
meson test # Run tests but tests needing test zim files.
```

If the automated tests fail or timeout, you need to be aware that some
tests need up to 16GB of memory. You can skip those specific tests with:
```bash
SKIP_BIG_MEMORY_TEST=1 meson test
```

Installation
------------

If you want to install the Libzim and the headers you just have
compiled on your system, here we go:
```bash
ninja -C build install
```

You might need to run the command as root (or using `sudo`), depending
where you want to install the libraries. After the installation
succeeded, you may need to run ldconfig (as root).

Uninstallation
------------

If you want to uninstall the Libzim:
```bash
ninja -C build uninstall
```

Like for the installation, you might need to run the command as root
(or using `sudo`).

Troubleshooting
---------------

If you need to install Meson "manually":
```bash
virtualenv -p python3 ./ # Create virtualenv
source bin/activate      # Activate the virtualenv
pip3 install meson       # Install Meson
hash -r                  # Refresh bash paths
```

If you need to install Ninja "manually":
```bash
git clone git://github.com/ninja-build/ninja.git
cd ninja
git checkout release
./configure.py --bootstrap
mkdir ../bin
cp ninja ../bin
cd ..
```

If the compilation still fails, you might need to get a more recent
version of a dependency than the one packaged by your Linux
distribution. Try then with a source tarball distributed by the
problematic upstream project or even directly from the source code
repository.

License
-------

[GPLv2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html) or
later, see [COPYING](COPYING) for more details.
