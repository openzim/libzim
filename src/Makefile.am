AM_CPPFLAGS=-I$(top_builddir)/include

lib_LTLIBRARIES = libzim.la

libzim_la_SOURCES = \
	article.cpp \
	bunzip2stream.cpp \
	bzip2.cpp \
	bzip2stream.cpp \
	cluster.cpp \
	deflatestream.cpp \
	dirent.cpp \
	file.cpp \
	fileheader.cpp \
	fileimpl.cpp \
	inflatestream.cpp \
	qunicode.cpp \
	unicode.cpp \
	uuid.cpp \
	zintstream.cpp

FOO = \
	articlesearch.cpp \
	files.cpp \
	indexarticle.cpp \
	search.cpp \
	unicode.cpp \
	zintstream.cpp

libzim_la_LDFLAGS=-lcxxtools -lz -lbz2
