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

namespace
{


void print_warning(const char* text)
{
  g_warning("%s", text);
}

void print_warning(const char* text, const Glib::ustring& what)
{
  g_warning(text, what.c_str());
}

/*
void print_invalid_value_warning(const Glib::ustring& value, const Glib::ustring& key)
{
  g_warning("Error in configuration file: invalid value `%s' for key `%s'",
            value.c_str(), key.c_str());
}
*/


struct NickValuePair
{
  const char* nick;
  int         value;
};

int enum_from_string(const Glib::ustring& value, const NickValuePair* value_map)
{
  const NickValuePair* p = value_map;

  for(; p->nick != 0; ++p)
  {
    if(value.raw() == p->nick)
      return p->value;
  }

  //print_invalid_value_warning(value);

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

void color_from_string(Gdk::Color& color, const Glib::ustring& value)
{
  color.parse(value);
    //print_invalid_value_warning(value, key);
}

} // anonymous namespace


namespace Regexxer
{

/**** Regexxer::ConfigData *************************************************/

const NickValuePair toolbar_style_values[] =
{
  { "icons",      Gtk::TOOLBAR_ICONS      },
  { "text",       Gtk::TOOLBAR_TEXT       },
  { "both",       Gtk::TOOLBAR_BOTH       },
  { "both_horiz", Gtk::TOOLBAR_BOTH_HORIZ },
  { 0,            Gtk::TOOLBAR_BOTH_HORIZ }
};

bool ConfigData::read_config_entry(const Glib::ustring& key, Glib::ustring& value)
{
  //Set the value of the key in GConf:
  const Glib::ustring key_complete = gconf_dir_path_ + key;

  try
  {
    value = refClient_->get_string(key_complete);
    return true;
  }
  catch(const Gnome::Conf::Error& ex)
  {
    print_warning("Error while retrieving configuration: %s", ex.what().c_str());
    return false;
  }
}

void ConfigData::write_config_entry(const Glib::ustring& key, const Glib::ustring& value)
{
  //Set the value of the key in GConf:
  const Glib::ustring key_complete = gconf_dir_path_ + key;
  refClient_->set(key_complete, value);
}

ConfigData::ConfigData()
:
  textview_font     ("Monospace"),
  match_color       ("orange"),
  current_color     ("yellow"),
  toolbar_style     (Gtk::TOOLBAR_BOTH_HORIZ),
  fallback_encoding ("ISO-8859-15"),
  gconf_dir_path_("/apps/gnome/regexxer")
{
  //Connect to GConf:
  refClient_ = Gnome::Conf::Client::get_default_client();
  refClient_->add_dir(gconf_dir_path_);
}

ConfigData::~ConfigData()
{}

void ConfigData::load()
{
  try
  {   
    read_config_entry("textview_font", textview_font);

    Glib::ustring color_string;
    read_config_entry("match_color", color_string);
    color_from_string(match_color, color_string);
    
    read_config_entry("current_match_color", color_string);
    color_from_string(current_color, color_string);

    Glib::ustring style_string;
    read_config_entry("toolbar_style", style_string);
    toolbar_style = Gtk::ToolbarStyle(enum_from_string(style_string, toolbar_style_values));

    Glib::ustring value_string; 
    read_config_entry("fallback_encoding", value_string);
    set_fallback_encoding_from_string(value_string);
  }
  catch(const Gnome::Conf::Error& error)
  {
    print_warning("Failed to read configuration: %s", error.what());
  }
}

void ConfigData::save()
{
  try
  {
    write_config_entry("textview_font", textview_font);
    write_config_entry("match_color", Util::color_to_string(match_color));
    write_config_entry("current_match_color", Util::color_to_string(current_color));
    write_config_entry("toolbar_style", enum_to_string(toolbar_style, toolbar_style_values));
    write_config_entry("fallback_encoding", get_string_from_fallback_encoding());
  }
  catch(const Gnome::Conf::Error& error)
  {
    print_warning("Failed to write configuration: %s", error.what());
  }
}

void ConfigData::set_fallback_encoding_from_string(const Glib::ustring& value)
{
  if(!value.empty()) //Use the default fallback encoding if there is nothing in the config settings.
  {
    std::string encoding = value.raw();

    if(Util::validate_encoding(encoding))
    {
      std::transform(encoding.begin(), encoding.end(), encoding.begin(), &Glib::Ascii::toupper);
      fallback_encoding = encoding;
    }
    else
    {
      //print_invalid_value_warning(value, "fallback_encoding");
    }
  }
}

Glib::ustring ConfigData::get_string_from_fallback_encoding() const
{
  const Glib::ustring encoding (fallback_encoding);
  g_return_val_if_fail(encoding.is_ascii(), "");

  return encoding;
}

} // namespace Regexxer

