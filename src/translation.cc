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

#include <config.h>
#if ENABLE_NLS
# include <libintl.h>
#endif

#include "translation.h"

#include <glib.h>
#include <glibmm.h>
#include <cstring>

#if ENABLE_NLS
void Util::initialize_gettext(const char* domain, const char* localedir)
{
  bindtextdomain(domain, localedir);
# if HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset(domain, "UTF-8");
# endif
  textdomain(domain);
}
#else
void Util::initialize_gettext(const char*, const char*)
{}
#endif /* !ENABLE_NLS */

const char* Util::translate(const char* msgid)
{
#if ENABLE_NLS
  return gettext(msgid);
#else
  return msgid;
#endif
}

Glib::ustring Util::compose_argv(const char* format, int argc, const Glib::ustring *const * argv)
{
  const std::string::size_type format_size = std::strlen(format);
  std::string::size_type       result_size = format_size;

  // Guesstimate the final string size.
  for (int i = 0; i < argc; ++i)
    result_size += argv[i]->raw().size();

  std::string result;
  result.reserve(result_size);

  const char* start = format;

  while (const char* stop = std::strchr(start, '%'))
  {
    if (stop[1] == '%')
    {
      result.append(start, stop - start + 1);
      start = stop + 2;
    }
    else
    {
      const int index = Glib::Ascii::digit_value(stop[1]) - 1;

      if (index >= 0 && index < argc)
      {
        result.append(start, stop - start);
        result += argv[index]->raw();
        start = stop + 2;
      }
      else
      {
        const char *const next = (stop[1] != '\0') ? g_utf8_next_char(stop + 1) : (stop + 1);

        // Copy invalid substitutions literally to the output.
        result.append(start, next - start);

        g_warning("invalid substitution \"%s\" in format string \"%s\"",
                  result.c_str() + result.size() - (next - stop), format);
        start = next;
      }
    }
  }

  result.append(start, format + format_size - start);

  return result;
}
