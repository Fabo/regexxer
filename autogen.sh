#! /bin/sh

# Copyright (c) 2004-2007  Daniel Elstner  <daniel.kitta@gmail.com>
#
# This file is part of regexxer.
#
# regexxer is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# regexxer is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with regexxer; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.

if test "x$NOCONFIGURE$*" = x
then
  echo "I am going to run $srcdir/configure with no arguments -- if you"
  echo "wish to pass any to it, please specify them on the $0 command line."
fi

# Let the user override the default choice of tools.
autoconf=$AUTOCONF
autoheader=$AUTOHEADER
aclocal=$ACLOCAL
automake=$AUTOMAKE

if test -z "$aclocal" || test -z "$automake"
then
  # Prefer explicitely versioned executables.
  for version in 1.10 1.9 1.8
  do
    if "aclocal-$version"  --version </dev/null >/dev/null 2>&1 && \
       "automake-$version" --version </dev/null >/dev/null 2>&1
    then
      aclocal=aclocal-$version
      automake=automake-$version
      break
    fi
  done
fi

test -n "$autoconf"   || autoconf=autoconf
test -n "$autoheader" || autoheader=autoheader
test -n "$aclocal"    || aclocal=aclocal
test -n "$automake"   || automake=automake

( # Enter a subshell to temporarily change the working directory.
  cd "$srcdir" || exit 1

  # Explicitely delete some old cruft, which seems to be
  # more reliable than --force options and the like.
  rm -f m4/codeset.m4 m4/gettext.m4 m4/glibc21.m4 m4/iconv.m4 m4/intltool.m4 m4/isc-posix.m4
  rm -f m4/lcmessage.m4 m4/lib-ld.m4 m4/lib-link.m4 m4/lib-prefix.m4 m4/progtest.m4
  rm -f intltool-extract.in intltool-merge.in intltool-update.in po/Makefile.in.in
  rm -f ABOUT-NLS acconfig.h config.cache config.guess config.rpath config.sub
  rm -f depcomp install-sh missing mkinstalldirs
  rm -rf autom4te.cache

  #WARNINGS=all; export WARNINGS
  (set -x) </dev/null >/dev/null 2>&1 && set -x

  glib-gettextize --copy				|| exit 1
  intltoolize --automake --copy --force			|| exit 1
  $aclocal -I m4 $ACLOCAL_FLAGS				|| exit 1
  $autoconf						|| exit 1
  $autoheader						|| exit 1
  $automake --add-missing --copy $AUTOMAKE_FLAGS	|| exit 1
) || exit 1

if test -z "$NOCONFIGURE"
then
  echo "+ $srcdir/configure $*"
  "$srcdir/configure" "$@" || exit 1
  echo
  echo 'Now type "make" to compile regexxer.'
fi

exit 0
