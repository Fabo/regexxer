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


pixmapdir = $(datadir)/pixmaps
dist_pixmap_DATA = regexxer.png

stockimages = stock_menu_about.png stock_save_all_16.png stock_save_all_24.png
dist_noinst_DATA = $(stockimages)

BUILT_SOURCES = stockimages.h
CLEANFILES    = stockimages.h

stockimages.h: $(stockimages)
	build_list=''; \
	list='$(stockimages)'; for file in $$list ; do \
	  name=`echo $$file | sed 's|\..*$$||'`; \
	  build_list="$$build_list $$name $(srcdir)/$$file"; \
	done; \
	$(GDK_PIXBUF_CSOURCE) --extern --raw --build-list $$build_list > $@

