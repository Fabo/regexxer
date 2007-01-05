## Copyright (c) 2004-2007  Daniel Elstner  <daniel.kitta@gmail.com>
##
## This file is part of regexxer.
##
## regexxer is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## regexxer is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with regexxer; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

AC_INIT([regexxer], [0.9], [daniel.kitta@gmail.com], [regexxer])
AC_PREREQ([2.55])

AC_CONFIG_SRCDIR([src/pcreshell.cc])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.8 -Wall gnu no-define nostdinc check-news])
AC_SUBST([ACLOCAL_AMFLAGS], ['-I m4 ${ACLOCAL_FLAGS}'])

AC_PROG_CC()
AC_PROG_CXX()

AC_PROG_INTLTOOL([0.23])
ALL_LINGUAS=`cat "$srcdir/po/LINGUAS"`
GETTEXT_PACKAGE=regexxer
AC_SUBST([CONFIG_STATUS_DEPENDENCIES], ['$(top_srcdir)/po/LINGUAS'])
AC_SUBST([GETTEXT_PACKAGE])
AM_GLIB_GNU_GETTEXT()

PKG_CHECK_MODULES([REGEXXER_MODULES],
                  [gtkmm-2.4 >= 2.4.0 libglademm-2.4 >= 2.4.0 gconfmm-2.6 >= 2.6.1 libpcre >= 5.0])

REGEXXER_PKG_PATH_PROG([GDK_PIXBUF_CSOURCE], [gdk-pixbuf-2.0], [gdk-pixbuf-csource])
REGEXXER_PKG_PATH_PROG([GTK_UPDATE_ICON_CACHE], [gtk+-2.0], [gtk-update-icon-cache])
REGEXXER_PKG_PATH_PROG([GCONFTOOL], [gconf-2.0], [gconftool-2])

AM_GCONF_SOURCE_2()
AC_SUBST([REGEXXER_GCONF_DIRECTORY], ['/apps/regexxer'])

AC_LANG([C++])
REGEXXER_LIB_POPT()
REGEXXER_LINK_EXPORT_DYNAMIC([REGEXXER_EXPORT_DYNAMIC])
REGEXXER_LINK_VERSION_SCRIPT([REGEXXER_VERSION_SCRIPT], [src/exported-symbols])
REGEXXER_ARG_ENABLE_WARNINGS()

AC_CONFIG_FILES([Makefile po/Makefile.in])
AC_OUTPUT()
