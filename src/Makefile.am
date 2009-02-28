AM_CPPFLAGS=-I$(top_builddir)/include

lib_LTLIBRARIES = libzim.la

libzim_la_SOURCES = \
	article.cpp \
	articlesearch.cpp \
	bunzip2stream.cpp \
	bzip2.cpp \
	bzip2stream.cpp \
	deflatestream.cpp \
	dirent.cpp \
	file.cpp \
	fileheader.cpp \
	fileimpl.cpp \
	files.cpp \
	indexarticle.cpp \
	inflatestream.cpp \
	qunicode.cpp \
	search.cpp \
	unicode.cpp \
	zintstream.cpp

libzim_la_LDFLAGS=-lcxxtools -lz -lbz2
