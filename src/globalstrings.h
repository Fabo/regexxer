/* $Id$
 *
 * Copyright (c) 2004  Daniel Elstner  <daniel.elstner@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License VERSION 2 as
 * published by the Free Software Foundation.  You are not allowed to
 * use any other version of the license; unless you got the explicit
 * permission from the author to do so.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef REGEXXER_GLOBALSTRINGS_H_INCLUDED
#define REGEXXER_GLOBALSTRINGS_H_INCLUDED

#include <glib/gutils.h>  /* for G_DIR_SEPARATOR_S */


namespace Regexxer
{

/*
 * Thanks to the GNU compiler/linker, these namespaced string constants
 * are no less efficient than string literals or preprocessor #defines.
 * The final executable contains exactly one copy of each string literal,
 * so there's no need to define them in a separate object file.
 */
const char *const conf_key_textview_font        = REGEXXER_GCONF_DIRECTORY "/textview_font";
const char *const conf_key_match_color          = REGEXXER_GCONF_DIRECTORY "/match_color";
const char *const conf_key_current_match_color  = REGEXXER_GCONF_DIRECTORY "/current_match_color";
const char *const conf_key_toolbar_style        = REGEXXER_GCONF_DIRECTORY "/toolbar_style";
const char *const conf_key_fallback_encoding    = REGEXXER_GCONF_DIRECTORY "/fallback_encoding";

const char *const glade_aboutdialog_filename    = REGEXXER_PKGDATADIR G_DIR_SEPARATOR_S "aboutdialog.glade";
const char *const glade_mainwindow_filename     = REGEXXER_PKGDATADIR G_DIR_SEPARATOR_S "mainwindow.glade";
const char *const glade_prefdialog_filename     = REGEXXER_PKGDATADIR G_DIR_SEPARATOR_S "prefdialog.glade";

} // namespace Regexxer

#endif /* REGEXXER_GLOBALSTRINGS_H_INCLUDED */

