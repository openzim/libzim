zimlib
------

The `zimlib` library is the standard implementation of the ZIM
specification.  It is a library which implements read and write
methods for ZIM files.

Use the zimlib in your own software --- for example, reader
applications --- to make them ZIM-capable without the need having to
dig too much into the ZIM file format.

To build:
```
./autogen.sh
./configure
make
```

OSX compilation
---------------
On MacOSX, you'll need to install some packages from
[homebrew](http://brew.sh/):
```
brew update
brew install xz libmagic
```
You'll also want to add `/usr/local/include` to your search path,
for example:
```
./autogen.sh
./configure CFLAGS=-I/usr/local/include CXXFLAGS=-I/usr/local/include
make
```

License
-------

The `zimlib` library is released under the GPLv2 license
terms.
