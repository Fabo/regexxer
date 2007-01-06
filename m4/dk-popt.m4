## Copyright (c) 2004-2007  Daniel Elstner  <daniel.kitta@gmail.com>
##
## This file is part of danielk's Autostuff.
##
## danielk's Autostuff is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License as published
## by the Free Software Foundation; either version 2 of the License, or (at
## your option) any later version.
##
## danielk's Autostuff is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
## or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
## for more details.
##
## You should have received a copy of the GNU General Public License along
## with danielk's Autostuff; if not, write to the Free Software Foundation,
## Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#serial 20070105

## DK_LIB_POPT()
##
## Check whether the popt library and its header file popt.h are available.
## On success, set the output variable POPT_LIBS to "-lpopt".
##
AC_DEFUN([DK_LIB_POPT],
[dnl
AC_CACHE_CHECK([for libpopt], [dk_cv_have_lib_popt],
[
  DK_SH_VAR_PUSH([LIBS], ["-lpopt $LIBS"])
  AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[
#include <popt.h>
]], [[
static struct poptOption option_table[] = { POPT_TABLEEND };
poptContext context;
context = poptGetContext(0, 0, 0, option_table, 0);
poptFreeContext(context);
]])],
  [dk_cv_have_lib_popt=yes],
  [dk_cv_have_lib_popt=no])
  DK_SH_VAR_POP([LIBS])
])
AS_IF([test "x$dk_cv_have_lib_popt" != xyes], [AC_MSG_FAILURE([[
The popt library is required in order to compile $PACKAGE_NAME.
Please install your distribution's libpopt development package.
]])])
AC_SUBST([POPT_LIBS], ['-lpopt'])
])
