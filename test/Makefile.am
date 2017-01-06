AM_CPPFLAGS=-I$(top_srcdir)/include

noinst_PROGRAMS = zimlib-test
TESTS = zimlib-test

if WITH_ZLIB
    ZLIB_SOURCES = \
        zlibstream.cpp
endif

if WITH_BZIP2
    BZIP2_SOURCES = \
        bzip2stream.cpp
endif

if WITH_LZMA
    LZMA_SOURCES = \
        lzmastream.cpp
endif

zimlib_test_SOURCES = \
    cluster.cpp \
    dirent.cpp \
    header.cpp \
    main.cpp \
    template.cpp \
    uuid.cpp \
    zint.cpp \
    $(ZLIB_SOURCES) \
    $(BZIP2_SOURCES) \
    $(LZMA_SOURCES)

LDADD = $(top_builddir)/src/libzim.la
zimlib_test_LDFLAGS = -lcxxtools -lcxxtools-unit
