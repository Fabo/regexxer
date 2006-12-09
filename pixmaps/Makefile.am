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

icon48dir = $(datadir)/icons/hicolor/48x48/apps
dist_icon48_DATA = regexxer.png

stockimages = stock_save_all_16.png stock_save_all_24.png
dist_noinst_DATA = $(stockimages)

BUILT_SOURCES = stockimages.h
CLEANFILES    = stockimages.h

# use --extern here and include stockimages.h within an anonymous namespace
gdk_pixbuf_csource_command = $(GDK_PIXBUF_CSOURCE) --extern --raw

stockimages.h: $(stockimages)
	@build_list='--build-list'; \
	list='$(stockimages)'; for file in $$list; do \
	  name=`echo "$$file" | sed 's|^.*/||;s|\..*$$||;s|-|_|g'`; \
	  dir=; test -f "$$file" || dir='$(srcdir)/'; \
	  build_list="$$build_list $$name $$dir$$file"; \
	done; \
	echo " $(gdk_pixbuf_csource_command) $$build_list >$@"; \
	$(gdk_pixbuf_csource_command) $$build_list >$@
