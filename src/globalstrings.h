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
const char *const conf_schema                  = "org.regexxer";
const char *const conf_key_textview_font       = "textview-font";
const char *const conf_key_match_color         = "match-color";
const char *const conf_key_current_match_color = "current-match-color";
const char *const conf_key_fallback_encoding   = "fallback-encoding";
const char *const conf_key_substitution_patterns = "substitution-patterns";
const char *const conf_key_regex_patterns      = "regex-patterns";
const char *const conf_key_files_patterns      = "files-patterns";
const char *const conf_key_window_width        = "window-width";
const char *const conf_key_window_height       = "window-height";
const char *const conf_key_window_position_x   = "window-position-x";
const char *const conf_key_window_position_y   = "window-position-y";
const char *const conf_key_window_maximized    = "window-maximized";
const char *const conf_key_show_line_numbers   = "show-line-numbers";
const char *const conf_key_highlight_current_line = "highlight-current-line";
const char *const conf_key_auto_indentation    = "auto-indentation";
const char *const conf_key_draw_spaces         = "draw-space";

const char *const ui_mainwindow_filename       = REGEXXER_PKGDATADIR G_DIR_SEPARATOR_S
                                                 "mainwindow.ui";
const char *const ui_prefdialog_filename       = REGEXXER_PKGDATADIR G_DIR_SEPARATOR_S
                                                 "prefdialog.ui";

} // namespace Regexxer

#endif /* REGEXXER_GLOBALSTRINGS_H_INCLUDED */
