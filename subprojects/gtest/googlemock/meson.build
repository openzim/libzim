# pkg-config support for GMock is in the master
# branch of the upstream repository.
# In order to be consistent with the upstream
# naming and the library conventions, the
# following changes have been made:
#
#   gmock_mainless_dep -> gmock_dep
#   gmock_with_main_dep -> gmock_main_dep
#
# Both GMock dependencies now include GTest,
# as this mimicks the same behavior as the
# upstream repo.

gmock_incdir = include_directories('include', '.')
gmock_libsources = files('src/gmock-all.cc')
gmock_mainsources = files('src/gmock_main.cc')

gmock_dep = declare_dependency(
	include_directories : [gtest_incdir, gmock_incdir],
	sources : [gtest_libsources, gmock_libsources],
	dependencies : dependency('threads')
)

gmock_main_dep = declare_dependency(
	include_directories : [gtest_incdir, gmock_incdir],
	sources : [gtest_libsources, gmock_libsources, gmock_mainsources],
	dependencies : dependency('threads')
)
