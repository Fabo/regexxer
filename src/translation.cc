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

#include <config.h>
#if ENABLE_NLS
# include <libintl.h>
#endif

#include "translation.h"

#include <glib.h>
#include <glibmm.h>


namespace
{

Glib::ustring compose_impl(const Glib::ustring& format,
                           int argc, const Glib::ustring *const * argv)
{
  using Glib::ustring;

  ustring result;

  ustring::const_iterator       p    = format.begin();
  const ustring::const_iterator pend = format.end();

  while (p != pend)
  {
    gunichar uc = *p++;

    if (uc == '%' && p != pend)
    {
      uc = *p++;

      if (uc != '%')
      {
        const int index = Glib::Unicode::digit_value(uc) - 1;

        if (index >= 0 && index < argc)
        {
          result += *argv[index];
          continue;
        }

        const Glib::ustring buf (1, uc);

        g_warning("Util::compose(): invalid substitution `%%%s' in format string `%s'",
                  buf.c_str(), format.c_str());

        result += '%'; // print invalid substitutions literally
      }
    }

    result += uc;
  }

  return result;
}

} // anonymous namespace


#if ENABLE_NLS
void Util::initialize_gettext(const char* domain, const char* localedir)
{
  bindtextdomain(domain, localedir);
  bind_textdomain_codeset(domain, "UTF-8");
  textdomain(domain);
}
#else
void Util::initialize_gettext(const char*, const char*)
{}
#endif /* ENABLE_NLS */

const char* Util::translate(const char* msgid)
{
#if ENABLE_NLS
  return gettext(msgid);
#else
  return msgid;
#endif
}

Glib::ustring Util::compose(const Glib::ustring& format, const Glib::ustring& arg1)
{
  const Glib::ustring *const argv[] = { &arg1 };
  return compose_impl(format, G_N_ELEMENTS(argv), argv);
}

Glib::ustring Util::compose(const Glib::ustring& format, const Glib::ustring& arg1,
                                                         const Glib::ustring& arg2)
{
  const Glib::ustring *const argv[] = { &arg1, &arg2 };
  return compose_impl(format, G_N_ELEMENTS(argv), argv);
}

Glib::ustring Util::compose(const Glib::ustring& format, const Glib::ustring& arg1,
                                                         const Glib::ustring& arg2,
                                                         const Glib::ustring& arg3)
{
  const Glib::ustring *const argv[] = { &arg1, &arg2, &arg3 };
  return compose_impl(format, G_N_ELEMENTS(argv), argv);
}

