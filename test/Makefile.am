AM_CPPFLAGS=-I$(top_builddir)/include

noinst_PROGRAMS = zimlib-test

zimlib_test_SOURCES = \
    bzip2stream.cpp \
    cluster.cpp \
    dirent.cpp \
    header.cpp \
    main.cpp \
    template.cpp \
    zint.cpp \
    zlibstream.cpp

LDADD = $(top_builddir)/src/libzim.la
zimlib_test_LDFLAGS=-lcxxtools-unit
