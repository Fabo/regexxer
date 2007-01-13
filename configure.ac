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

AC_INIT([regexxer], [0.9], [http://regexxer.sourceforge.net/bugs], [regexxer])
AC_PREREQ([2.58])

AC_CONFIG_SRCDIR([ui/regexxer.desktop.in])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.8 -Wall gnu no-define nostdinc check-news])
AC_SUBST([ACLOCAL_AMFLAGS], ["-I m4 $ACLOCAL_FLAGS"])

AC_PROG_CC()
AC_PROG_CXX()

IT_PROG_INTLTOOL([0.35])
AC_SUBST([GETTEXT_PACKAGE], [regexxer])
AM_GLIB_GNU_GETTEXT()

# gmodule-export-2.0 adds -Wl,--export-dynamic to the linker flags
# so that libglade can get at the custom widget creation functions.
PKG_CHECK_MODULES([REGEXXER_MODULES],
                  [gmodule-export-2.0 >= 2.6.1 gtkmm-2.4 >= 2.6.0 libglademm-2.4 >= 2.4.0
                   gconfmm-2.6 >= 2.6.1 libpcre >= 5.0])

DK_PKG_PATH_PROG([GDK_PIXBUF_CSOURCE], [gdk-pixbuf-2.0], [gdk-pixbuf-csource])
DK_PKG_PATH_PROG([GTK_UPDATE_ICON_CACHE], [gtk+-2.0], [gtk-update-icon-cache])
DK_PKG_PATH_PROG([GCONFTOOL], [gconf-2.0], [gconftool-2])

AM_GCONF_SOURCE_2()

AC_LANG([C++])

# If the linker supports it, use a version script to limit the effect
# of --export-dynamic to the functions which need to be exported.
DK_LINK_VERSION_SCRIPT([REGEXXER_VERSION_SCRIPT], [src/exported-symbols])

# Disabling -Wnon-virtual-dtor is nasty, but unfortunately required due to
# a bug in gconfmm that triggers this warning.  Not disabling this warning
# would make --enable-warnings=fatal unusable.
DK_ARG_ENABLE_WARNINGS([REGEXXER_WARNING_FLAGS],
                       [-Wall -w1 -Wno-non-virtual-dtor],
                       [-pedantic -Wall -Wextra -w1 -Wno-non-virtual-dtor])

AC_CONFIG_FILES([Makefile po/Makefile.in])
AC_OUTPUT()
