#! /bin/sh
# Regenerate the files autoconf / automake

if [ "`(uname -s) 2>/dev/null`" = "Darwin" ]
then
    glibtoolize --force
else
    libtoolize --force --automake
fi

rm -f config.cache config.log
aclocal -I m4
autoheader
autoconf
automake -a
