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


## REGEXXER_CXX_HAS_STD_LOCALE()
##
## Check whether the C++ environment supports std::locale.
## If true, #define REGEXXER_HAVE_STD_LOCALE 1.
##
AC_DEFUN([REGEXXER_CXX_HAS_STD_LOCALE],
[
AC_CACHE_CHECK(
  [whether the C++ library supports std::locale],
  [regexxer_cv_cxx_has_std_locale],
[
  AC_LANG_PUSH([C++])
  AC_LINK_IFELSE(
  [
    AC_LANG_PROGRAM(
    [[
      #include <iostream>
      #include <locale>
    ]],[[
      std::cout.imbue(std::locale(""));
    ]])
  ],
    [regexxer_cv_cxx_has_std_locale=yes],
    [regexxer_cv_cxx_has_std_locale=no])
  AC_LANG_POP([C++])
])

if test "x$regexxer_cv_cxx_has_std_locale" = xyes; then
{
  AC_DEFINE([REGEXXER_HAVE_STD_LOCALE], [1], [Define to 1 if the C++ library supports std::locale.])
}
fi
])


## REGEXXER_ARG_ENABLE_WARNINGS()
##
## Provide the --enable-warnings configure argument, set to 'minimum'
## by default.
##
AC_DEFUN([REGEXXER_ARG_ENABLE_WARNINGS],
[
AC_REQUIRE([AC_PROG_CXX])

AC_ARG_ENABLE([warnings], AC_HELP_STRING(
  [--enable-warnings=@<:@none|minimum|maximum|hardcore@:>@],
  [Control compiler pickyness. @<:@default=minimum@:>@]),
  [regexxer_enable_warnings=$enableval],
  [regexxer_enable_warnings=minimum])

AC_MSG_CHECKING([for compiler warning flags to use])

warning_flags=

case $regexxer_enable_warnings in
  minimum|yes) warning_flags='-Wall' ;;
  maximum)     warning_flags='-pedantic -W -Wall' ;;
  hardcore)    warning_flags='-pedantic -W -Wall -Werror' ;;
esac

tested_flags=

if test "x$warning_flags" != x; then
{
  AC_LANG_PUSH([C++])
  AC_LANG_CONFTEST([AC_LANG_SOURCE([[int foo() { return 0; }]])])
  conftest_source="conftest.${ac_ext:-cc}"

  for flag in $warning_flags
  do
    # Test whether the compiler accepts the flag.  GCC doesn't bail
    # out when given an unsupported flag but prints a warning, so
    # check the compiler output instead.
    regexxer_cxx_out=`$CXX $tested_flags $flag -c $conftest_source 2>&1 || echo failed`
    rm -f "conftest.$OBJEXT"
    test "x$regexxer_cxx_out" = x && tested_flags=${tested_flags:+"$tested_flags "}$flag
  done

  rm -f "$conftest_source"
  regexxer_cxx_out=
  AC_LANG_POP([C++])
}
fi

if test "x$tested_flags" != x
then
  for flag in $tested_flags
  do
    case " $CXXFLAGS " in
      *" $flag "*) ;; # don't add flags twice
      *)           CXXFLAGS=${CXXFLAGS:+"$CXXFLAGS "}$flag ;;
    esac
  done
else
  tested_flags=none
fi

AC_MSG_RESULT([${tested_flags}])
])

