if WITH_CXXTOOLS
    ZIMDUMP_DIR = zimDump

if MAKE_UNITTEST
    UNITTEST_DIR = test

test: all
	test/zimlib-test

endif

endif

SUBDIRS = \
	src \
	include \
    $(ZIMDUMP_DIR) $(UNITTEST_DIR)

