# pkg-config support for GTest is in the master
# branch of the upstream repository.
# In order to be consistent with the upstream
# naming and the library conventions, the
# following changes have been made:
#
#   gtest_mainless_dep -> gtest_dep
#   gtest_with_main_dep -> gtest_main_dep

gtest_incdir = include_directories('include', '.')
gtest_libsources = files('src/gtest-all.cc')
gtest_mainsources = files('src/gtest_main.cc')

gtest_dep = declare_dependency(
	include_directories : gtest_incdir,
	sources : gtest_libsources,
	dependencies : dependency('threads')
)

gtest_main_dep = declare_dependency(
	include_directories : gtest_incdir,
	sources : [gtest_libsources, gtest_mainsources],
	dependencies : dependency('threads')
)
