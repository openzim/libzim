AC_INIT(zimlib, 1.4, [Tommi Maekitalo <tommi@tntnet.org>])
AM_INIT_AUTOMAKE([subdir-objects])

abi_current=2
abi_revision=0
abi_age=0
sonumber=${abi_current}:${abi_revision}:${abi_age}
AC_SUBST(sonumber)

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_SRCDIR([src/file.cpp])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_CHECK_HEADER([lzma.h], , AC_MSG_ERROR([lzma header files not found]))
AC_CHECK_FUNCS([stat64 lseek64 open64])

AC_LANG(C++)

AX_COMPILER_VENDOR
AS_IF([test "$ax_cv_cxx_compiler_vendor" = "ibm"],
      [CXXFLAGS="$CXXFLAGS -qrtti -qlanglvl=newexcp -D__NOLOCK_ON_INPUT -D__NOLOCK_ON_OUTPUT"])
AX_CHECK_COMPILE_FLAG([-Wno-long-long], [CXXFLAGS="$CXXFLAGS -Wno-long-long"])
AX_CHECK_COMPILE_FLAG([-Wall], [CXXFLAGS="$CXXFLAGS -Wall"])
AX_CHECK_COMPILE_FLAG([-pedantic], [CXXFLAGS="$CXXFLAGS -pedantic"])

#
# cxxtools
#
AC_ARG_WITH([cxxtools],
  AS_HELP_STRING([--with-cxxtools=yes|no], [use cxxtools (default:yes)]),
  [with_cxxtools=$withval],
  [with_cxxtools=no])

if test "$with_cxxtools" = "yes"
then
  AC_DEFINE(WITH_CXXTOOLS, [1], [defined if cxxtools is used])
  AC_CHECK_HEADER([cxxtools/condition.h], , AC_MSG_ERROR([cxxtools headers not found]))
fi

AM_CONDITIONAL([WITH_CXXTOOLS], test "$with_cxxtools" = yes)

#
# cache size
#
AC_ARG_WITH([cluster-cache-size],
  AS_HELP_STRING([--with-cluster-cache-size=number], [set cluster cache size to number (default:16)]),
  [cluster_cache_size=$withval],
  [cluster_cache_size=16])

AC_DEFINE_UNQUOTED(CLUSTER_CACHE_SIZE, $cluster_cache_size, [set cluster cache size to number of cached chunks])

AC_ARG_WITH([dirent-cache-size],
  AS_HELP_STRING([--with-dirent-cache-size=number], [set dirent cache size to number (default:512)]),
  [dirent_cache_size=$withval],
  [dirent_cache_size=512])

AC_DEFINE_UNQUOTED(DIRENT_CACHE_SIZE, $dirent_cache_size, [set dirent cache size to number of cached chunks])

#
# compression algorithms
#

# zlib
AC_ARG_ENABLE([zlib],
  AS_HELP_STRING([--enable-zlib], [add support for zlib compression (disabled by default)]),
  [enable_zlib=$enableval],
  [enable_zlib=no])

if test "$enable_zlib" = "yes"
then
    AC_CHECK_HEADER([zlib.h], , AC_MSG_ERROR([zlib header not found]))
    AC_DEFINE(ENABLE_ZLIB, [1], [defined if zlib compression is enabled])
fi

AM_CONDITIONAL(WITH_ZLIB, test "$enable_zlib" = "yes")

# bzip2
AC_ARG_ENABLE([bzip2],
  AS_HELP_STRING([--enable-bzip2], [add support for bzip2 compression (disabled by default)]),
  [enable_bzip2=$enableval],
  [enable_bzip2=no])

if test "$enable_bzip2" = "yes"
then
    AC_CHECK_HEADER([bzlib.h], , AC_MSG_ERROR([bzip2 header files not found]))
    AC_DEFINE(ENABLE_BZIP2, [1], [defined if bzip2 compression is enabled])
fi

AM_CONDITIONAL(WITH_BZIP2, test "$enable_bzip2" = "yes")

# lzma
AC_ARG_ENABLE([lzma],
  AS_HELP_STRING([--disable-lzma], [add support for lzma compression (enabled by default)]),
  [enable_lzma=$enableval],
  [enable_lzma=yes])

if test "$enable_lzma" = "yes"
then
    AC_CHECK_HEADER([lzma.h], , AC_MSG_ERROR([lzma header files not found]))
    AC_DEFINE(ENABLE_LZMA, [1], [defined if lzma compression is enabled])
fi

AM_CONDITIONAL(WITH_LZMA, test "$enable_lzma" = "yes")

#
# unittest
#
AC_ARG_ENABLE([unittest],
  AS_HELP_STRING([--enable-unittest], [enable building unittest (cxxtools needed)]),
  [enable_unittest=$enableval],
  [enable_unittest=no])

if test "$enable_unittest" = "yes"
then
  AC_CHECK_HEADER([cxxtools/unit/test.h], , AC_MSG_ERROR([cxxtools unit test headers not found]))
fi

AM_CONDITIONAL(MAKE_UNITTEST, test "$enable_unittest" = "yes")

#
# benchmark tool
#
AC_ARG_ENABLE([benchmark],
  AS_HELP_STRING([--enable-benchmark], [build benchmark tool zimBench]),
  [enable_benchmark=$enableval],
  [enable_benchmark=disable_benchmark])

AM_CONDITIONAL(MAKE_BENCHMARK, test "$enable_benchmark" = "yes")

#
# lzma memory usage
#
AC_ARG_WITH([lzma-memory-size],
  AS_HELP_STRING([--with-lzma-memory-size=number], [set lzma uncompress memory in MB (default:128)]),
  [lzma_memory_size=$withval],
  [lzma_memory_size=128])

AC_DEFINE_UNQUOTED(LZMA_MEMORY_SIZE, $lzma_memory_size, [set lzma uncompress memory size to number of MB])

#
# output
#
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/tools/Makefile
  include/Makefile
  test/Makefile
  examples/Makefile
  ])

AC_OUTPUT
