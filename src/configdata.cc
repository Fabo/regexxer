/* $Id$
 *
 * Copyright (c) 2002  Daniel Elstner  <daniel.elstner@gmx.net>
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

#include "configdata.h"
#include "stringutils.h"

#include <glib.h>
#include <glibmm.h>
#include <algorithm>

#include <config.h>

#if HAVE_UMASK

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if !HAVE_MODE_T
typedef int mode_t;
#endif

#endif /* HAVE_UMASK */


namespace
{

#if HAVE_UMASK

class ScopedUmask
{
private:
  mode_t old_mask_;

  ScopedUmask(const ScopedUmask&);
  ScopedUmask& operator=(const ScopedUmask&);

public:
  explicit ScopedUmask(mode_t mask) : old_mask_ (umask(mask)) {}
  ~ScopedUmask()                    { umask(old_mask_); }
};

#endif /* HAVE_UMASK */


struct NickValuePair
{
  const char* nick;
  int         value;
};

int enum_from_string(const Glib::ustring& value, const Glib::ustring& key,
                     const NickValuePair* value_map)
{
  const NickValuePair* p = value_map;

  for(; p->nick != 0; ++p)
  {
    if(value.raw() == p->nick)
      return p->value;
  }

  g_warning("Error in configuration file: invalid value `%s' for key `%s'",
            value.c_str(), key.c_str());

  return p->value; // return some fallback value
}

Glib::ustring enum_to_string(int value, const NickValuePair* value_map)
{
  for(const NickValuePair* p = value_map; p->nick != 0; ++p)
  {
    if(value == p->value)
      return p->nick;
  }

  g_return_val_if_reached("");
}


const NickValuePair menutool_mode_values[] =
{
  { "menu_and_tool", Regexxer::MODE_MENU_AND_TOOL },
  { "menu_only",     Regexxer::MODE_MENU_ONLY     },
  { "tool_only",     Regexxer::MODE_TOOL_ONLY     },
  { 0,               Regexxer::MODE_MENU_AND_TOOL }
};

const NickValuePair toolbar_style_values[] =
{
  { "icons",      Gtk::TOOLBAR_ICONS      },
  { "text",       Gtk::TOOLBAR_TEXT       },
  { "both",       Gtk::TOOLBAR_BOTH       },
  { "both_horiz", Gtk::TOOLBAR_BOTH_HORIZ },
  { 0,            Gtk::TOOLBAR_BOTH_HORIZ }
};


void print_warning(const char* text)
{
  g_warning("%s", text);
}

void print_warning(const char* text, const Glib::ustring& what)
{
  g_warning(text, what.c_str());
}

Glib::RefPtr<Glib::IOChannel> open_config_file(const std::string& mode)
{
  const std::string filename = Glib::build_filename(Glib::get_home_dir(), ".regexxer");

  return Glib::IOChannel::create_from_file(filename, mode);
}

bool read_config_entry(const Glib::RefPtr<Glib::IOChannel>& channel,
                       Glib::ustring& key, Glib::ustring& value)
{
  using Glib::ustring;

  ustring line;

  while(channel->read_line(line) == Glib::IO_STATUS_NORMAL)
  {
    ustring::const_iterator pbegin = line.begin();
    ustring::const_iterator pend   = line.end();

    Util::trim_whitespace(pbegin, pend);

    if(pbegin == pend || *pbegin == '#')
      continue;

    // Use byte-wise std::string iterators because it's faster.
    // It's possible to do this when searching for an ASCII character.
    const ustring::const_iterator passign (std::find(pbegin.base(), pend.base(), '='));

    if(passign == pend)
    {
      print_warning("Error in configuration file: missing `='");
      continue;
    }

    ustring::const_iterator pend_key = passign;
    Util::trim_whitespace(pbegin, pend_key);

    if(pend_key == pbegin)
    {
      print_warning("Error in configuration file: missing key before `='");
      continue;
    }

    ustring::const_iterator pbegin_value = passign;
    Util::trim_whitespace(++pbegin_value, pend);

    if(pbegin_value == pend)
    {
      print_warning("Error in configuration file: missing value after `='");
      continue;
    }

    key.assign(pbegin, pend_key);
    value.assign(pbegin_value, pend);

    return true;
  }

  return false;
}

void write_config_entry(const Glib::RefPtr<Glib::IOChannel>& channel,
                        const Glib::ustring& key, const Glib::ustring& value)
{
  channel->write(key);
  channel->write("=");
  channel->write(value);
  channel->write("\n");
}

} // anonymous namespace


namespace Regexxer
{

/**** Regexxer::ConfigData *************************************************/

ConfigData::ConfigData()
:
  menutool_mode     (MODE_MENU_AND_TOOL),
  toolbar_style     (Gtk::TOOLBAR_BOTH_HORIZ),
  fallback_encoding ("ISO-8859-15")
{}

ConfigData::~ConfigData()
{}

void ConfigData::load()
{
  try
  {
    const Glib::RefPtr<Glib::IOChannel> channel = open_config_file("r");

    Glib::ustring key;
    Glib::ustring value;

    while(read_config_entry(channel, key, value))
    {
      if(key.raw() == "menutool_mode")
        menutool_mode = MenuToolMode(enum_from_string(value, key, menutool_mode_values));
      else if(key.raw() == "toolbar_style")
        toolbar_style = Gtk::ToolbarStyle(enum_from_string(value, key, toolbar_style_values));
      else if(key.raw() == "fallback_encoding")
        set_fallback_encoding_from_string(value);
      else
        print_warning("Error in configuration file: unknown key `%s'", key);
    }
  }
  catch(const Glib::FileError& error)
  {
    if(error.code() != Glib::FileError::NO_SUCH_ENTITY)
      print_warning("Failed to open configuration file: %s", error.what());
  }
  catch(const Glib::Error& error)
  {
    print_warning("Failed to read configuration file: %s", error.what());
  }
}

void ConfigData::save()
{
  try
  {
#if HAVE_UMASK
    ScopedUmask scoped_umask (077); // set -rw------- for newly created files
#endif
    const Glib::RefPtr<Glib::IOChannel> channel = open_config_file("w");

    channel->write("# regexxer configuration file\n");

    write_config_entry(channel, "menutool_mode", enum_to_string(menutool_mode, menutool_mode_values));
    write_config_entry(channel, "toolbar_style", enum_to_string(toolbar_style, toolbar_style_values));
    write_config_entry(channel, "fallback_encoding", get_string_from_fallback_encoding());

    channel->close(); // close explicitly because it might fail
  }
  catch(const Glib::FileError& error)
  {
    print_warning("Failed to open configuration file: %s", error.what());
  }
  catch(const Glib::Error& error)
  {
    print_warning("Failed to write configuration file: %s", error.what());
  }
}

void ConfigData::set_fallback_encoding_from_string(const Glib::ustring& value)
{
  std::string encoding = value.raw();

  if(Util::validate_encoding(encoding))
  {
    std::transform(encoding.begin(), encoding.end(), encoding.begin(), &Glib::Ascii::toupper);
    fallback_encoding = encoding;
  }
  else
  {
    print_warning("Error in configuration file: invalid value `%s' for key `fallback_encoding'", value);
  }
}

Glib::ustring ConfigData::get_string_from_fallback_encoding() const
{
  const Glib::ustring encoding (fallback_encoding);
  g_return_val_if_fail(encoding.is_ascii(), "");

  return encoding;
}

} // namespace Regexxer

