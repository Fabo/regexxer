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

bin_PROGRAMS = regexxer

regexxer_SOURCES =		\
	aboutdialog.cc		\
	aboutdialog.h		\
	controller.cc		\
	controller.h		\
	filebuffer.cc		\
	filebuffer.h		\
	filebufferundo.cc	\
	filebufferundo.h	\
	fileio.cc		\
	fileio.h		\
	fileshared.cc		\
	fileshared.h		\
	filetree.cc		\
	filetree.h		\
	filetreeprivate.cc	\
	filetreeprivate.h	\
	globalstrings.h		\
	main.cc			\
	mainwindow.cc		\
	mainwindow.h		\
	miscutils.cc		\
	miscutils.h		\
	pcreshell.cc		\
	pcreshell.h		\
	prefdialog.cc		\
	prefdialog.h		\
	sharedptr.cc		\
	sharedptr.h		\
	signalutils.cc		\
	signalutils.h		\
	statusline.cc		\
	statusline.h		\
	stringutils.cc		\
	stringutils.h		\
	translation.cc		\
	translation.h		\
	undostack.cc		\
	undostack.h

dist_noinst_DATA = exported-symbols

global_defines = \
-DREGEXXER_DATADIR=\""$(datadir)"\" \
-DREGEXXER_PKGDATADIR=\""$(pkgdatadir)"\" \
-DREGEXXER_GCONF_DIRECTORY=\""$(REGEXXER_GCONF_DIRECTORY)"\" \
-DGTKMM_DISABLE_DEPRECATED

AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir) $(global_defines) $(PKG_CFLAGS) $(PCRE_CFLAGS)
regexxer_DEPENDENCIES = exported-symbols
regexxer_LDFLAGS = $(REGEXXER_VERSION_SCRIPT)
regexxer_LDADD = $(LIBINTL) $(PKG_LIBS) $(PCRE_LIBS) $(POPT_LIBS)

