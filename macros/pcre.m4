## $Id$
##
## Copyright (c) 2002  Daniel Elstner  <daniel.elstner@gmx.net>
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License VERSION 2 as
## published by the Free Software Foundation.  You are not allowed to
## use any other version of the license; unless you got the explicit
## permission from the author to do so.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


## PCRE_CHECK_VERSION(min_version)
##
## Run pcre-config to determine the libpcre version number and
## bail out if it is insufficient.  Also retrieve the necessary
## compiler flags and store them in PCRE_CFLAGS rpt. PCRE_LIBS.
##
AC_DEFUN([PCRE_CHECK_VERSION],
[
m4_if([$1],, [AC_FATAL([argument required])])

AC_ARG_VAR([PCRE_CONFIG], [path to pcre-config script])
AC_PATH_PROG([PCRE_CONFIG], [pcre-config])

AS_IF([test "x$PCRE_CONFIG" = x],
[
AC_MSG_FAILURE([[
*** pcre-config is missing.  Please install your distribution's
*** libpcre development package and then try again.
]])
])

AC_MSG_CHECKING([[for libpcre >= ]$1])

pcre_version_string=`$PCRE_CONFIG --version 2>&5`

d='@<:@0123456789@:>@'
pcre_transform='s/^\('$d$d'*\)\.\('$d$d'*\)\.*\('$d'*\).*$/\1 \\* 1000000 + \2 \\* 1000 + 0\3/p'
pcre_required=`echo "$1" | sed -n "$pcre_transform" 2>&5`
pcre_version=`echo "$pcre_version_string" | sed -n "$pcre_transform" 2>&5`

AS_IF([eval "expr $pcre_version \\>= $pcre_required" >/dev/null 2>&5],
      [pcre_version_ok=yes],
      [pcre_version_ok=no])

AC_MSG_RESULT([$pcre_version_ok])

AS_IF([test "x$pcre_version_ok" = xno],
[
AC_MSG_FAILURE([[
*** libpcre ]$1[ or higher is required, but you only have
*** version $pcre_version_string installed.  Please upgrade and try again.
]])
])

AC_MSG_CHECKING([[PCRE_CFLAGS]])
PCRE_CFLAGS=`$PCRE_CONFIG --cflags | sed 's,-I/usr/include$,,;s,-I/usr/include ,,g' 2>&5`
AC_MSG_RESULT([${PCRE_CFLAGS}])
AC_SUBST([PCRE_CFLAGS])

AC_MSG_CHECKING([[PCRE_LIBS]])
PCRE_LIBS=`$PCRE_CONFIG --libs | sed 's,-L/usr/lib$,,;s,-L/usr/lib ,,g' 2>&5`
AC_MSG_RESULT([${PCRE_LIBS}])
AC_SUBST([PCRE_LIBS])
])


## PCRE_CHECK_UTF8()
##
## Run a test program to determine whether PCRE was compiled with
## UTF-8 support.  If it wasn't, bail out with an error message.
##
AC_DEFUN([PCRE_CHECK_UTF8],
[
AC_REQUIRE([PCRE_CHECK_VERSION])

AC_CACHE_CHECK(
  [whether libpcre was compiled with UTF-8 support],
  [pcre_cv_has_utf8_support],
[
  pcre_saved_CPPFLAGS=$CPPFLAGS
  pcre_saved_LIBS=$LIBS
  CPPFLAGS="$CPPFLAGS $PCRE_CFLAGS"
  LIBS="$LIBS $PCRE_LIBS"

  AC_RUN_IFELSE(
  [
    AC_LANG_PROGRAM(
    [[
#     include <stdio.h>
#     include <stdlib.h>
#     include <pcre.h>
    ]],[[
      const char* errmessage = 0;
      int erroffset = 0;

      if (pcre_compile(".", PCRE_UTF8, &errmessage, &erroffset, 0))
        exit(0);

      fprintf(stderr, "%s\n", errmessage);
      exit(1);
    ]])
  ],
    [pcre_cv_has_utf8_support=yes],
    [pcre_cv_has_utf8_support=no],
    [pcre_cv_has_utf8_support="cross compile: assuming yes"])

  CPPFLAGS=$pcre_saved_CPPFLAGS
  LIBS=$pcre_saved_LIBS
])

AS_IF([test "x$pcre_cv_has_utf8_support" = xno],
[
AC_MSG_FAILURE([[
*** Sorry, the PCRE library installed on your system doesn't support
*** UTF-8 encoding.  Please install a libpcre package which includes
*** support for UTF-8.  Note that if you compile libpcre from source
*** you have to pass the --enable-utf8 flag to its ./configure script.
]])
])
])

