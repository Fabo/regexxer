#! /bin/sh -e
test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.
(
  cd "$srcdir"
  export AUTOPOINT='intltoolize --automake --copy'
  autoreconf --force --install
)
test -n "$NOCONFIGURE" || "$srcdir/configure" "$@"
