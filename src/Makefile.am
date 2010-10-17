AM_CPPFLAGS=-I$(top_builddir)/include

lib_LTLIBRARIES = libzim.la

if WITH_ZLIB
ZLIB_SOURCES = \
	deflatestream.cpp \
	inflatestream.cpp
ZLIB_LDFLAGS = -lz
endif

if WITH_BZIP2
BZIP2_SOURCES = \
	bunzip2stream.cpp \
	bzip2.cpp \
	bzip2stream.cpp
BZIP2_LDFLAGS = -lbz2
endif

if WITH_LZMA
LZMA_SOURCES = \
	lzmastream.cpp \
	unlzmastream.cpp
LZMA_LDFLAGS = -llzma
endif

if WITH_CXXTOOLS
CXXTOOLS_LDFLAGS = -lcxxtools
else
NOCXXTOOLS_SOURCES = \
	md5.c \
	md5stream.cpp
endif

libzim_la_SOURCES = \
	article.cpp \
	articlesearch.cpp \
	cluster.cpp \
	dirent.cpp \
	envvalue.cpp \
	file.cpp \
	fileheader.cpp \
	fileimpl.cpp \
	fstream.cpp \
	indexarticle.cpp \
	ptrstream.cpp \
	search.cpp \
	template.cpp \
	unicode.cpp \
	uuid.cpp \
	zintstream.cpp \
	$(ZLIB_SOURCES) \
	$(BZIP2_SOURCES) \
	$(LZMA_SOURCES) \
	$(NOCXXTOOLS_SOURCES)

noinst_HEADERS = \
	envvalue.h \
	log.h \
	md5.h \
	md5stream.h \
	ptrstream.h

libzim_la_LDFLAGS = $(ZLIB_LDFLAGS) $(BZIP2_LDFLAGS) $(LZMA_LDFLAGS) $(CXXTOOLS_LDFLAGS)
