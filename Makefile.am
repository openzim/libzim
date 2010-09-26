if WITH_CXXTOOLS

TOOLS_DIR = src/tools

if MAKE_UNITTEST

UNITTEST_DIR = test

test: all
	test/zimlib-test

endif

endif

SUBDIRS = \
	src \
	include \
	$(TOOLS_DIR) $(UNITTEST_DIR)
