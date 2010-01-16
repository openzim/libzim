AM_CPPFLAGS=-I$(top_builddir)/include
if MAKE_BENCHMARK
  ZIMBENCH = zimbench
endif
bin_PROGRAMS = zimdump zimsearch $(ZIMBENCH)
zimdump_SOURCES = zimDump.cpp
zimsearch_SOURCES = zimSearch.cpp
zimbench_SOURCES = zimBench.cpp
AM_LDFLAGS = -lcxxtools
LDADD = $(top_builddir)/src/libzim.la
