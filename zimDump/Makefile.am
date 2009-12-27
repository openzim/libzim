AM_CPPFLAGS=-I$(top_builddir)/include
if MAKE_BENCHMARK
  ZIMBENCH = zimBench
endif
bin_PROGRAMS = zimDump zimSearch $(ZIMBENCH)
zimDump_SOURCES = zimDump.cpp
zimSearch_SOURCES = zimSearch.cpp
zimBench_SOURCES = zimBench.cpp
AM_LDFLAGS = -lcxxtools
LDADD = $(top_builddir)/src/libzim.la
