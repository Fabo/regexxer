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
const char* translate(const char* msgid) G_GNUC_CONST;

Glib::ustring compose(const Glib::ustring& format, const Glib::ustring& arg1);
Glib::ustring compose(const Glib::ustring& format, const Glib::ustring& arg1,
                                                   const Glib::ustring& arg2);
Glib::ustring compose(const Glib::ustring& format, const Glib::ustring& arg1,
                                                   const Glib::ustring& arg2,
                                                   const Glib::ustring& arg3);

} // namespace Util

#endif /* REGEXXER_TRANSLATION_H_INCLUDED */

