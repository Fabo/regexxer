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

#ifndef REGEXXER_TRANSLATION_H_INCLUDED
#define REGEXXER_TRANSLATION_H_INCLUDED

#include <glib/gmacros.h>
#include <glibmm/ustring.h>

#ifndef gettext_noop
# define gettext_noop(s) (s)
#endif

#define _(s) ::Util::translate(s)
#define N_(s) gettext_noop(s)

namespace Util
{

void initialize_gettext(const char* domain, const char* localedir);
const char* translate(const char* msgid) G_GNUC_PURE G_GNUC_FORMAT(1);

Glib::ustring compose(const Glib::ustring& format, const Glib::ustring& arg1);
Glib::ustring compose(const Glib::ustring& format, const Glib::ustring& arg1,
                                                   const Glib::ustring& arg2);
Glib::ustring compose(const Glib::ustring& format, const Glib::ustring& arg1,
                                                   const Glib::ustring& arg2,
                                                   const Glib::ustring& arg3);

} // namespace Util

#endif /* REGEXXER_TRANSLATION_H_INCLUDED */
