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


AC_INIT([regexxer],[0.6],[daniel.elstner@gmx.net],[regexxer])
AC_PREREQ(2.59)

AC_REVISION([$Revision$])
AC_CONFIG_SRCDIR([src/pcreshell.cc])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.7 gnu no-define nostdinc check-news])

AC_PROG_CC()
AC_PROG_CXX()

AC_CHECK_HEADERS([sys/types.h sys/stat.h])
AC_CHECK_FUNCS([umask])
AC_CHECK_TYPES([mode_t])

PKG_CHECK_MODULES([MM_DEPS], [gtkmm-2.4 >= 2.4.0 gconfmm-2.6 >= 2.6.0 gnome-vfsmm-2.6 >= 2.6.0])
AC_SUBST([MM_DEPS_CFLAGS])
AC_SUBST([MM_DEPS_LIBS])

gdk_pixbuf_exec_prefix=`${PKG_CONFIG:-"pkg-config"} --variable=exec_prefix gdk-pixbuf-2.0 2>&5`
prog_search_path=${gdk_pixbuf_exec_prefix:+"$gdk_pixbuf_exec_prefix/bin$PATH_SEPARATOR"}$PATH

AC_PATH_PROG([GDK_PIXBUF_CSOURCE], [gdk-pixbuf-csource], [not found], [$prog_search_path])

if test "x$GDK_PIXBUF_CSOURCE" = "xnot found"; then
{
AC_MSG_ERROR([[
*** Ooops, couldn't find gdk-pixbuf-csource.  Actually this should
*** never happen at this point, which means your system is really broken.
]])
}
fi

PCRE_CHECK_VERSION([3.9])
PCRE_CHECK_UTF8()

REGEXXER_CXX_HAS_STD_LOCALE()
REGEXXER_ARG_ENABLE_WARNINGS()

AC_CONFIG_FILES([
  Makefile
  pixmaps/Makefile
  src/Makefile
])


AC_OUTPUT

