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


ACLOCAL_AMFLAGS = -I macros ${ACLOCAL_FLAGS}
DISTCHECK_CONFIGURE_FLAGS = --disable-schemas-install --enable-warnings=hardcore

SUBDIRS = pixmaps ui src po

desktop_in_files    = regexxer.desktop.in
desktopdir          = $(datadir)/applications
desktop_DATA        = regexxer.desktop

@INTLTOOL_DESKTOP_RULE@

schemas_in_files    = regexxer.schemas.in
schemasdir          = $(GCONF_SCHEMA_FILE_DIR)
schemas_DATA        = regexxer.schemas

@INTLTOOL_SCHEMAS_RULE@

dist_macros         = macros/cxx.m4 macros/misc.m4 macros/pcre.m4
dist_intltool       = intltool-extract.in intltool-merge.in intltool-update.in

dist_noinst_SCRIPTS = autogen.sh
dist_noinst_DATA    = $(dist_macros) $(dist_intltool) $(desktop_in_files) $(schemas_in_files)

CLEANFILES          = $(desktop_DATA) $(schemas_DATA)
DISTCLEANFILES      = intltool-extract intltool-merge intltool-update

gconftool_command   = GCONF_CONFIG_SOURCE='$(GCONF_SCHEMA_CONFIG_SOURCE)' $(GCONFTOOL)

if GCONF_SCHEMAS_INSTALL
install-data-hook: install-schemas
uninstall-hook: uninstall-schemas
else
install-data-hook:
uninstall-hook:
endif

install-schemas: $(schemas_DATA)
	@$(POST_INSTALL)
	$(gconftool_command) --makefile-install-rule $(schemas_DATA)
	@echo; \
	echo "*** NOTE ***"; \
	echo "If you executed src/regexxer before running 'make install' as privileged user,"; \
	echo "it might be necessary to restart the GConf daemon now in order for the default"; \
	echo "configuration values to take effect.  Do to so, run (as ordinary user):"; \
	echo; \
	echo "  gconftool-2 --shutdown; gconftool-2 --spawn"; \
	echo

uninstall-schemas: $(schemas_DATA)
	@$(NORMAL_UNINSTALL)
	$(gconftool_command) --makefile-uninstall-rule $(schemas_DATA)

