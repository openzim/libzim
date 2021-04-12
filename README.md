ZIM library
===========

The ZIM library is the reference implementation for the ZIM file
format. It's a solution to read and write ZIM files on many systems
and architectures. More information about the ZIM format and the
openZIM project at https://openzim.org/.

[![Release](https://img.shields.io/github/v/tag/openzim/libzim?label=release&sort=semver)](https://download.openzim.org/release/libzim/)
[![Reporitories](https://img.shields.io/repology/repositories/zimlib?label=repositories)](https://github.com/openzim/libzim/wiki/Repology)
[![License](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Build](https://github.com/openzim/libzim/workflows/CI/badge.svg?query=branch%3Amaster)](https://github.com/openzim/libzim/actions?query=branch%3Amaster)
[![Doc](https://readthedocs.org/projects/libzim/badge/?style=flat)](https://libzim.readthedocs.io/en/latest/?badge=latest)
[![Codecov](https://codecov.io/gh/openzim/libzim/branch/master/graph/badge.svg)](https://codecov.io/gh/openzim/libzim)
[![CodeFactor](https://www.codefactor.io/repository/github/openzim/libzim/badge)](https://www.codefactor.io/repository/github/openzim/libzim)

Disclaimer
----------

This document assumes you have a little knowledge about software
compilation. If you experience difficulties with the dependencies or
with the ZIM library compilation itself, we recommend to have a look
to [kiwix-build](https://github.com/kiwix/kiwix-build).

Preamble
--------

Although the ZIM library can be compiled/cross-compiled on/for many
systems, the following documentation explains how to do it on POSIX
ones. It is primarily though for GNU/Linux systems and has been tested
on recent releases of Ubuntu and Fedora.

Dependencies
------------

The ZIM library relies on many third parts software libraries. They
are prerequisites to the Kiwix library compilation. Following
libraries need to be available:

* [LZMA](https://tukaani.org/lzma/) (package `liblzma-dev` on Ubuntu)
* [ICU](http://site.icu-project.org/) (package `libicu-dev` on Ubuntu)
* [Zstd](https://facebook.github.io/zstd/) (package `libzstd-dev` on Ubuntu)
* [Xapian](https://xapian.org/) - optional (package `libxapian-dev` on Ubuntu)
* [UUID](http://e2fsprogs.sourceforge.net/) (package `uuid-dev` on Ubuntu)
* [Google Test](https://github.com/google/googletest) - optional (package `googletest` on Ubuntu)
* [zim-testing-suite](https://github.com/openzim/zim-testing-suite) - optional, data for unittests.

To build the documentations you need the packages :

* [Doxygen](https://www.doxygen.nl)
* Python packages [Sphinx](https://www.sphinx-doc.org), [breathe](https://breathe.readthedocs.io) and [exhale](https://exhale.readthedocs.io).

These dependencies may or may not be packaged by your operating
system. They may also be packaged but only in an older version. The
compilation script will tell you if one of them is missing or too old.
In the worse case, you will have to download and compile a more recent
version by hand.

If you want to install these dependencies locally, then ensure that
meson (through `pkg-config`) will properly find them.

Environment
-------------

The ZIM library builds using [Meson](https://mesonbuild.com/) version
0.43 or higher. Meson relies itself on Ninja, Pkg-config and few other
compilation tools.

Install first the few common compilation tools:
* Meson
* Ninja
* Pkg-config

These tools should be packaged if you use a cutting edge operating
system. If not, have a look to the "Troubleshooting" section.

Compilation
-----------

Once all dependencies are installed, you can compile ZIM library with:
```bash
meson . build
ninja -C build
```

By default, it will compile dynamic linked libraries. All binary files
will be created in the `build` directory created automatically by
Meson. If you want statically linked libraries, you can add
`--default-library=static` option to the Meson command.

If you want to build the documentation, we need to pass the `-Ddoc=true` option and run the `doc` target:
```bash
meson . build -Ddoc=true
ninja -C build doc
```

Depending of you system, `ninja` may be called `ninja-build`.

By default, libzim tries to compile with Xapian (and will generate an
error if Xapian is not found).  You can build without xapian by
passing the option `-Dwith_xapian=false` :
```bash
meson . build -Dwith_xapian=false
ninja -C build doc
```

If libzim is compiled without Xapian, all search API are removed.  You
can test if an installed version of libzim is compiled with or without
xapian by testing the define `LIBZIM_WITH_XAPIAN`.

Testing
-------

The unit test need testing zim files.
They are located in the repository [zim-testing-suite](https://github.com/openzim/zim-testing-suite).

Decompress the zim files in https://github.com/openzim/zim-testing-suite/releases/download/v0.1/zim-testing-suite-0.1.tar.gz in a directory
and set a environment variable pointing to this directory.

Then, you can run :
```bash
meson test
```

If the automated tests fail or timeout, you need to be aware that this
test suite needs up to 16GB of memory. You can skip this specific tests with:
```bash
SKIP_BIG_MEMORY_TEST=1 meson test
```

Installation
------------

If you want to install the libzim and the headers you just have
compiled on your system, here we go:
```bash
ninja -C build install
```

You might need to run the command as root (or using `sudo`), depending
where you want to install the libraries. After the installation
succeeded, you may need to run ldconfig (as root).

Uninstallation
------------

If you want to uninstall the libzim:
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

[GPLv2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html) or later, see [COPYING](COPYING) for more details.
