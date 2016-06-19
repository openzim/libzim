AM_CPPFLAGS=-I$(top_srcdir)/include -I$(top_srcdir)/src
if MAKE_BENCHMARK
  ZIMBENCH = zimbench
endif
bin_PROGRAMS = zimdump zimsearch $(ZIMBENCH)
zimdump_SOURCES = zimDump.cpp
zimsearch_SOURCES = zimSearch.cpp
zimbench_SOURCES = zimBench.cpp
LDADD = $(top_builddir)/src/libzim.la
