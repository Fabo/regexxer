# Copyright (C) 2002 Daniel Elstner <daniel.elstner@gmx.net>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AC_INIT([regexxer], [0.1], [daniel.elstner@gmx.net], [regexxer])
AC_CONFIG_SRCDIR([src/filelist.cc])
AC_CANONICAL_BUILD()
AC_CANONICAL_HOST()
AC_CANONICAL_TARGET()

#
# If the user didn't predefine any flags, set them to `space'.
# This prevents autoconf from setting them to defaults.
#
#test "x${CFLAGS}" = "x"   && CFLAGS=' '
#test "x${CXXFLAGS}" = "x" && CXXFLAGS=' '
#test "x${LDFLAGS}" = "x"  && LDFLAGS=' '

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([gnu dist-bzip2 no-define nostdinc])

AC_PROG_CC()
AC_PROG_CXX()

PKG_CHECK_MODULES(GTKMM, sigc++-1.2 >= 1.1.13 gtkmm-2.0 >= 1.3.23)
AC_SUBST([GTKMM_CFLAGS])
AC_SUBST([GTKMM_LIBS])

PCRE_CHECK_VERSION([3.4])
PCRE_CHECK_UTF8()

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  pixmaps/Makefile
])

AC_OUTPUT()

