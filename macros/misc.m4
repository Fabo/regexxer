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
## Like AC_PATH_PROG(variable, executable, [not found], <extra_path>), where
## <extra_path> is set to the contents of $PATH prepended by the package's
## binary executable directory.  This should catch even the weirdest setups.
## An error message is generated if the executable cannot be found anywhere
## in the resulting path.
##
AC_DEFUN([REGEXXER_PKG_PATH_PROG],
[
AC_REQUIRE([PKG_CHECK_MODULES])

pkg_search_path=$PATH
pkg_exec_prefix=`${PKG_CONFIG-"pkg-config"} --variable=exec_prefix "$2" 2>&5`
test "x$pkg_exec_prefix" = x || pkg_search_path="$pkg_exec_prefix/bin$PATH_SEPARATOR$pkg_search_path"

AC_PATH_PROG([$1], [$3], [not found], [$pkg_search_path])

AS_IF([test "x$$1" = "xnot found"],
[
AC_MSG_ERROR([[
*** Ooops, couldn't find ]$3[.  Actually this should
*** never happen at this point, which means your system is really broken.
]])
])
])

