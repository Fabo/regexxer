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


## REGEXXER_ARG_ENABLE_WARNINGS()
##
## Provide the --enable-warnings configure argument, set to 'minimum'
## by default.
##
AC_DEFUN([REGEXXER_ARG_ENABLE_WARNINGS],
[
AC_REQUIRE([AC_PROG_CXX])

AC_ARG_ENABLE([warnings], AS_HELP_STRING(
  [--enable-warnings=@<:@none|minimum|maximum|hardcore@:>@],
  [Control compiler pickyness. @<:@default=minimum@:>@]),
  [regexxer_enable_warnings=$enableval],
  [regexxer_enable_warnings=minimum])

AC_MSG_CHECKING([for compiler warning flags to use])

case $regexxer_enable_warnings in
  minimum|yes) warning_flags='-Wall' ;;
  maximum)     warning_flags='-pedantic -W -Wall' ;;
  hardcore)    warning_flags='-pedantic -W -Wall -Werror' ;;
  *)           warning_flags= ;;
esac

tested_flags=

AS_IF([test "x$warning_flags" != x],
[
  AC_LANG_PUSH([C++])
  AC_LANG_CONFTEST([AC_LANG_SOURCE([[int foo() { return 0; }]])])
  conftest_source="conftest.${ac_ext-cc}"

  for flag in $warning_flags
  do
    # Test whether the compiler accepts the flag.  GCC doesn't bail
    # out when given an unsupported flag but prints a warning, so
    # check the compiler output instead.
    regexxer_cxx_out=`$CXX $tested_flags $flag -c $conftest_source 2>&1 || echo failed`
    rm -f "conftest.$OBJEXT"
    AS_IF([test "x$regexxer_cxx_out" = x],
          [AS_IF([test "x$tested_flags" = x],
                 [tested_flags=$flag],
                 [tested_flags="$tested_flags $flag"])])
  done

  rm -f "$conftest_source"
  regexxer_cxx_out=
  AC_LANG_POP([C++])
])

AS_IF([test "x$tested_flags" != x],
[
  for flag in $tested_flags
  do
    case " $CXXFLAGS " in
      *" $flag "*) ;; # don't add flags twice
      "  ")        CXXFLAGS=$flag ;;
      *)           CXXFLAGS="$CXXFLAGS $flag" ;;
    esac
  done
],[
  tested_flags=none
])

AC_MSG_RESULT([${tested_flags}])
])


## REGEXXER_CXXLINK_VERSION_SCRIPT(filename)
##
## Check whether the C++ linker accepts the --version-script flag.
## On success, assign the flag complete with filename to the output
## variable REGEXXER_VERSION_SCRIPT.  The filename should be a path
## relative to the top source directory.
##
AC_DEFUN([REGEXXER_CXXLINK_VERSION_SCRIPT],
[
m4_if([$1],, [AC_FATAL([argument required])])

AC_CACHE_CHECK(
  [whether the C++ linker accepts -Wl,--version-script],
  [regexxer_cv_cxxlink_version_script],
[
  AC_LANG_PUSH([C++])
  regexxer_save_ldflags=$LDFLAGS
  LDFLAGS="$LDFLAGS -Wl,--version-script=$srcdir/$1"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([], [])],
                 [regexxer_cv_cxxlink_version_script=yes],
                 [regexxer_cv_cxxlink_version_script=no])
  LDFLAGS=$regexxer_save_ldflags
  AC_LANG_POP([C++])
])

AS_IF([test "x$regexxer_cv_cxxlink_version_script" = xyes],
[
  REGEXXER_VERSION_SCRIPT='-Wl,--version-script=$(top_srcdir)/'"$1"
],[
  REGEXXER_VERSION_SCRIPT=
])

AC_SUBST([REGEXXER_VERSION_SCRIPT])
])

