AM_CPPFLAGS=-I$(top_builddir)/include

lib_LTLIBRARIES = libzim.la

libzim_la_SOURCES = \
	article.cpp \
	articlesearch.cpp \
	bunzip2stream.cpp \
	bzip2.cpp \
	bzip2stream.cpp \
	cluster.cpp \
	deflatestream.cpp \
	dirent.cpp \
	file.cpp \
	fileheader.cpp \
	fileimpl.cpp \
	indexarticle.cpp \
	inflatestream.cpp \
	ptrstream.cpp \
	search.cpp \
	template.cpp \
	unicode.cpp \
	uuid.cpp \
	zintstream.cpp

#include_HEADERS = \
#	ptrstream.h

libzim_la_LDFLAGS=-lz -lbz2

if WITH_CXXTOOLS
libzim_la_LDFLAGS+=-lcxxtools
endif
