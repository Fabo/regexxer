#! /bin/sh -e
test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.
(
  cd "$srcdir" &&
  AUTOPOINT='intltoolize --automake --copy' autoreconf --force --install --verbose
) || exit
test -n "$NOCONFIGURE" || "$srcdir/configure" "$@"
