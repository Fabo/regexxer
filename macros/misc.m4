## $Id$
##
## Copyright (c) 2004  Daniel Elstner  <daniel.elstner@gmx.net>
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


## REGEXXER_PKG_PATH_PROG(variable, package, executable)
##
## Like AC_PATH_PROG(variable, executable,, <extra_path>), where <extra_path>
## is set to the contents of $PATH prepended by the package's binary executable
## directory.  This should catch even the weirdest setups.  An error message is
## generated if the executable cannot be found anywhere in the resulting path.
##
AC_DEFUN([REGEXXER_PKG_PATH_PROG],
[
m4_if([$3],, [AC_FATAL([3 arguments required])])
AC_REQUIRE([PKG_CHECK_MODULES])

pkg_search_path=$PATH
pkg_exec_prefix=`${PKG_CONFIG-"pkg-config"} --variable=exec_prefix "$2" 2>&5`
test "x$pkg_exec_prefix" = x || pkg_search_path="$pkg_exec_prefix/bin$PATH_SEPARATOR$pkg_search_path"

AC_PATH_PROG([$1], [$3],, [$pkg_search_path])

AS_IF([test "x$$1" = x],
[
AC_MSG_FAILURE([[
*** Ooops, couldn't find ]$3[.  Actually this should
*** never happen at this point, which means your system is really broken.
]])
])
])


## REGEXXER_LIB_POPT()
##
## Check whether the popt library and its header file <popt.h> are
## available.  On success, the output variable POPT_LIBS will be set.
##
AC_DEFUN([REGEXXER_LIB_POPT],
[
AC_CACHE_CHECK(
  [for libpopt],
  [regexxer_cv_has_lib_popt],
[
  regexxer_save_LIBS=$LIBS
  LIBS="$LIBS -lpopt"

  AC_LINK_IFELSE(
  [
    AC_LANG_PROGRAM(
    [[
#     include <popt.h>
    ]],[[
      static struct poptOption option_table@<:@@:>@ = { POPT_TABLEEND };
      poptContext context;

      context = poptGetContext(0, 0, 0, option_table, 0);
      poptFreeContext(context);
    ]])
  ],
    [regexxer_cv_has_lib_popt=yes],
    [regexxer_cv_has_lib_popt=no])

  LIBS=$regexxer_save_LIBS
])

AS_IF([test "x$regexxer_cv_has_lib_popt" = xno],
[
AC_MSG_FAILURE([[
*** The popt library is required in order to compile $PACKAGE.
*** Please install the libpopt development package of your distribution.
]])
])

AC_SUBST([POPT_LIBS], ['-lpopt'])
])

