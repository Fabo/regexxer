#! /bin/sh

# $Id$
#
# Copyright (c) 2002  Daniel Elstner  <daniel.elstner@gmx.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License VERSION 2 as
# published by the Free Software Foundation.  You are not allowed to
# use any other version of the license; unless you got the explicit
# permission from the author to do so.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


# Be Bourne compatible. (stolen from autoconf)
if test -n "${ZSH_VERSION+set}" && (emulate sh) >/dev/null 2>&1; then
  emulate sh
  NULLCMD=:
  # Zsh 3.x and 4.x performs word splitting on ${1+"$@"}, which
  # is contrary to our usage.  Disable this feature.
  alias -g '${1+"$@"}'='"$@"'
elif test -n "${BASH_VERSION+set}" && (set -o posix) >/dev/null 2>&1; then
  set -o posix
fi

PROJECT=regexxer

srcdir=`dirname "$0"`
test -z $srcdir && srcdir=.

origdir=`pwd`
cd "$srcdir"

ACLOCAL_FLAGS="-I ./macros $ACLOCAL_FLAGS"
AUTOMAKE_FLAGS="--add-missing --gnu $AUTOMAKE_FLAGS"

if test -z "$AUTOGEN_SUBDIR_MODE" && test -z "$*"
then
  echo "I am going to run ./configure with no arguments - if you wish "
  echo "to pass any to it, please specify them on the $0 command line."
fi

autoconf=autoconf
autoheader=autoheader
aclocal=aclocal
automake=automake

for suffix in "1.7" "1.6"
do
  if "$aclocal-$suffix"  --version </dev/null >/dev/null 2>&1 && \
     "$automake-$suffix" --version </dev/null >/dev/null 2>&1
  then
    aclocal="$aclocal-$suffix"
    automake="$automake-$suffix"
    break
  fi
done

rm -f config.guess config.sub depcomp install-sh missing mkinstalldirs
rm -f config.cache acconfig.h
rm -rf autom4te.cache

WARNINGS=all
export WARNINGS

echo "$aclocal $ACLOCAL_FLAGS"
"$aclocal" $ACLOCAL_FLAGS || exit 1

echo "$autoheader"
"$autoheader" || exit 1

echo "$automake $AUTOMAKE_FLAGS"
"$automake" $AUTOMAKE_FLAGS || exit 1

echo "$autoconf"
"$autoconf" || exit 1

cd "$origdir"

if test -z "$AUTOGEN_SUBDIR_MODE"
then
  echo "$srcdir/configure $*"
  "$srcdir/configure" "$@" || exit 1

  echo
  echo "Now type 'make' to compile $PROJECT."
fi

exit 0

