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


AC_INIT([regexxer], [0.7], [daniel.elstner@gmx.net], [regexxer])
AC_PREREQ([2.53])

AC_REVISION([$Revision$])
AC_CONFIG_SRCDIR([src/pcreshell.cc])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.7 gnu no-define nostdinc check-news])

AC_PROG_CC()
AC_PROG_CXX()

AM_GNU_GETTEXT([external])

PKG_CHECK_MODULES([PKG], [gtkmm-2.4 >= 2.4.0 libglademm-2.4 >= 2.4.0 gconfmm-2.6 >= 2.6.0])
AC_SUBST([PKG_CFLAGS])
AC_SUBST([PKG_LIBS])

PCRE_CHECK_VERSION([3.9])
PCRE_CHECK_UTF8()

REGEXXER_PKG_PATH_PROG([GDK_PIXBUF_CSOURCE], [gdk-pixbuf-2.0], [gdk-pixbuf-csource])
REGEXXER_PKG_PATH_PROG([GCONFTOOL], [gconf-2.0], [gconftool-2])

AM_GCONF_SOURCE_2()
AC_SUBST([REGEXXER_GCONF_DIRECTORY], ['/apps/regexxer'])

REGEXXER_CXX_HAS_STD_LOCALE()
REGEXXER_ARG_ENABLE_WARNINGS()

## This is a really, really nasty thing to do, but...
##
AC_DEFINE([REGEXXER_ENABLE_GCONFMM_VALUE_HACK], [1], [Define to 1 if this nasty hack is necessary.])
AH_BOTTOM(
[
#if REGEXXER_ENABLE_GCONFMM_VALUE_HACK
/*
 * Hardcore HACK:  Perform the copy which is missing in gconfmm
 * five minutes after twelve.
 */
namespace Util { bool has_broken_gconfmm_value_changed(); }

# define REGEXXER_GCONFMM_VALUE_HACK(value)					\
    if (Util::has_broken_gconfmm_value_changed())				\
      new(const_cast<Gnome::Conf::Value*>(&value))				\
          Gnome::Conf::Value(const_cast<GConfValue*>(value.gobj()), true)
#else
# define REGEXXER_GCONFMM_VALUE_HACK(value) (void)0
#endif
])

AC_CONFIG_FILES([
  Makefile
  pixmaps/Makefile
  ui/Makefile
  src/Makefile
  po/Makefile.in
])

AC_OUTPUT()

