ZIM library
=============

The ZIM library is the reference implementation for the openZIM file
format. It's a solution to read and write ZIM file on many systems and
architectures. More information about the ZIM format and the openZIM
project at http://www.openzim.org/

Disclaimer
----------

This document assumes you have a little knowledge about software
compilation. If you experience difficulties with the dependencies or
with the ZIM libary compilation itself, we recommend to have a look to
[kiwix-build](https://github.com/kiwix/kiwix-build).

Preamble
--------

Although the ZIM library can be compiled/cross-compiled on/for many
sytems, the following documentation explains how to do it on POSIX
ones. It is primarly though for GNU/Linux systems and has been tested
on recent releases of Ubuntu and Fedora.

Dependencies
------------

The Kiwix library relies on many third parts software libraries. They
are prerequisites to the Kiwix library compilation. Following
libraries need to be available:

* Z ................................................. http://zlib.net/
(package zlib1g-dev on Ubuntu)
* Bzip2 ......................................... http://www.bzip.org/
(package libbz2-dev on Ubuntu)
* Lzma ...................................... http://tukaani.org/lzma/
(package lzma-dev on Ubuntu)
* ICU ................................... http://site.icu-project.org/
(package libicu-dev on Ubuntu)
* Xapian ......................................... https://xapian.org/
(package libxapian-dev on Ubuntu)

These dependencies may or may not be packaged by your operating
system. They may also be packaged but only in an older version. The
compilation script will tell you if one of them is missing or too old.
In the worse case, you will have to download and compile a more recent
version by hand.

If you want to install these dependencies locally, then ensure that
meson (through pkg-config) will properly find them.

Environnement
-------------

The ZIM library builds using [Meson](http://mesonbuild.com/) version
0.34 or higher. Meson relies itself on Ninja, pkg-config and few other
compilation tools.

Install first the few common compilation tools:
* Automake
* Libtool
* Virtualenv
* Pkg-config

Then install Meson itself:
```
virtualenv -p python3 ./ # Create virtualenv
source bin/activate      # Activate the virtualenv
pip3 install meson       # Install Meson
hash -r                  # Refresh bash paths
```

Finally we need the Ninja tool (available in the $PATH). Here is a
solution to download, build and install it locally:

```
git clone git://github.com/ninja-build/ninja.git
cd ninja
git checkout release
./configure.py --bootstrap
mkdir ../bin
cp ninja ../bin
cd ..
```

Compilation
-----------

Once all dependencies are installed, you can compile ZIM library with:
```
mkdir build
meson . build
cd build
ninja
```

By default, it will compile dynamic linked libraries. If you want
statically linked libraries, you can add `--default-library=static`
option to the Meson command.

Depending of you system, `ninja` may be called `ninja-build`.

Installation
------------

If you want to install the libraries you just have compiled on your
system, here we go:

```
ninja install
cd ..
```

You might need to run the command as root, depending where you want to
install the libraries.

License
-------

GPLv2, see COPYING for more details.