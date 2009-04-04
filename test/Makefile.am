AM_CPPFLAGS=-I$(top_builddir)/include
noinst_PROGRAMS = zimlib-test
zimlib_test_SOURCES = main.cpp header.cpp dirent.cpp zlibstream.cpp bzip2stream.cpp cluster.cpp qunicode.cpp
LDADD = $(top_builddir)/src/libzim.la
zimlib_test_LDFLAGS=-lcxxtools-unit
