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

#ifndef REGEXXER_CONFIGDATA_H_INCLUDED
#define REGEXXER_CONFIGDATA_H_INCLUDED

#include <gconfmm.h>
#include <gdkmm/color.h>
#include <glibmm/ustring.h>
#include <gtkmm/toolbar.h>


namespace Regexxer
{

// This class is a bit strange because it originally read/wrote a text configuration file,
// but now it is implemented with GConf instead (which is much simpler).
class ConfigData
{
public:
  Glib::ustring     textview_font;
  Gdk::Color        match_color;
  Gdk::Color        current_color;
  Gtk::ToolbarStyle toolbar_style;
  std::string       fallback_encoding;

  ConfigData();
  ~ConfigData();

  void load();
  void save();

private:
  void set_fallback_encoding_from_string(const Glib::ustring& value);
  Glib::ustring get_string_from_fallback_encoding() const;

  bool read_config_entry(const Glib::ustring& key, Glib::ustring& value);
  void write_config_entry(const Glib::ustring& key, const Glib::ustring& value);

  Glib::RefPtr<Gnome::Conf::Client> refClient_;
  Glib::ustring gconf_dir_path_;
};

} // namespace Regexxer

#endif /* REGEXXER_CONFIGDATA_H_INCLUDED */

