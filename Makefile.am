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


ACLOCAL_AMFLAGS = -I $(top_srcdir)/macros
DISTCHECK_CONFIGURE_FLAGS = --disable-schemas-install --enable-warnings=hardcore

SUBDIRS = pixmaps ui src po

dist_noinst_SCRIPTS = autogen.sh config.rpath
dist_noinst_DATA    = macros/cxx.m4 macros/misc.m4 macros/pcre.m4

desktopdir          = $(datadir)/applications
dist_desktop_DATA   = regexxer.desktop

schemadir           = $(GCONF_SCHEMA_FILE_DIR)
dist_schema_DATA    = regexxer.schemas

gconftool_command   = GCONF_CONFIG_SOURCE='$(GCONF_SCHEMA_CONFIG_SOURCE)' $(GCONFTOOL)

if GCONF_SCHEMAS_INSTALL
install-data-hook: install-schemas
uninstall-hook: uninstall-schemas
else
install-data-hook:
uninstall-hook:
endif

install-schemas: $(dist_schema_DATA)
	@$(POST_INSTALL)
	@file='$(dist_schema_DATA)'; test -f "$$file" || file="$(srcdir)/$$file"; \
	echo " $(gconftool_command) --makefile-install-rule $$file"; \
	$(gconftool_command) --makefile-install-rule $$file
	@echo; \
	echo "*** NOTE ***"; \
	echo "If you executed src/regexxer before running 'make install' as privileged user,"; \
	echo "it might be necessary to restart the GConf daemon now in order for the default"; \
	echo "configuration values to take effect.  Do to so, run (as ordinary user):"; \
	echo; \
	echo "  gconftool-2 --shutdown; gconftool-2 --spawn"; \
	echo

uninstall-schemas:
	@$(NORMAL_UNINSTALL)
	@file='$(dist_schema_DATA)'; test -f "$$file" || file="$(srcdir)/$$file"; \
	echo " $(gconftool_command) --makefile-uninstall-rule $$file"; \
	$(gconftool_command) --makefile-uninstall-rule $$file

