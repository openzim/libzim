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
	files.cpp \
	fileheader.cpp \
	fileimpl.cpp \
	indexarticle.cpp \
	inflatestream.cpp \
	ptrstream.cpp \
	qunicode.cpp \
	search.cpp \
	unicode.cpp \
	uuid.cpp \
	zintstream.cpp

include_HEADERS = \
	ptrstream.h

libzim_la_LDFLAGS=-lcxxtools -lz -lbz2
