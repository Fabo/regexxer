/*
 * Copyright (c) 2002-2007  Daniel Elstner  <daniel.kitta@gmail.com>
 *
 * This file is part of regexxer.
 *
 * regexxer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * regexxer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with regexxer; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
const char *const conf_dir_application         = "/apps/regexxer";
const char *const conf_key_textview_font       = "/apps/regexxer/textview_font";
const char *const conf_key_match_color         = "/apps/regexxer/match_color";
const char *const conf_key_current_match_color = "/apps/regexxer/current_match_color";
const char *const conf_key_toolbar_style       = "/apps/regexxer/toolbar_style";
const char *const conf_key_fallback_encoding   = "/apps/regexxer/fallback_encoding";
const char *const conf_key_substitution_patterns = "/apps/regexxer/substitution_patterns";
const char *const conf_key_regex_patterns      = "/apps/regexxer/regex_patterns";
const char *const conf_key_files_patterns      = "/apps/regexxer/files_patterns";

const char *const ui_mainwindow_filename       = REGEXXER_PKGDATADIR G_DIR_SEPARATOR_S
                                                 "mainwindow.ui";
const char *const ui_prefdialog_filename       = REGEXXER_PKGDATADIR G_DIR_SEPARATOR_S
                                                 "prefdialog.ui";

} // namespace Regexxer

#endif /* REGEXXER_GLOBALSTRINGS_H_INCLUDED */
