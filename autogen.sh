#!/bin/sh

WANT_AUTOMAKE=1.7
export WANT_AUTOMAKE

echo "=============== running libtoolize --force --copy" &&
    libtoolize --force --copy &&
    echo "=============== running aclocal" &&
    aclocal &&
    echo "=============== running autoheader" &&
    autoheader &&
    echo "=============== running automake --add-missing --foreign" &&
    automake --add-missing --foreign &&
    echo "=============== running autoconf" &&
    autoconf &&
    echo "=============== done"

