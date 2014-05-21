#! /bin/sh
# Regenerate the files autoconf / automake

if [ "`(uname -s) 2>/dev/null`" = "Darwin" ]
then
    LIBTOOLIZE=glibtoolize
else
    LIBTOOLIZE=libtoolize
fi
$LIBTOOLIZE --force --automake

rm -f config.cache config.log
aclocal -I m4
autoheader
autoconf
automake -a
