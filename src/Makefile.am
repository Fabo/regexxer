## Copyright (C) 2002 Daniel Elstner <daniel.elstner@gmx.net>
##
## This file is free software; as a special exception the author gives
## unlimited permission to copy and/or distribute it, with or without
## modifications, as long as this notice is preserved.
##
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
## implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


##SUBDIRS = pixmaps util vbox voice mywidgets

bin_PROGRAMS = regexxer

regexxer_SOURCES =	\
filebuffer.cc		\
filebuffer.h		\
filelist.cc		\
filelist.h		\
main.cc			\
mainwindow.cc		\
mainwindow.h		\
pcreshell.cc		\
pcreshell.h		\
sharedptr.cc		\
sharedptr.h		\
stringutils.cc		\
stringutils.h

DEFS     = @DEFS@ -DREGEXXER_DATADIR="\"$(datadir)\""
INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_srcdir)/src $(PCRE_CFLAGS) $(GTKMM_CFLAGS)
LIBS     = $(PCRE_LIBS) $(GTKMM_LIBS)

