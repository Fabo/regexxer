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

#ifndef REGEXXER_PREFDIALOG_H_INCLUDED
#define REGEXXER_PREFDIALOG_H_INCLUDED

#include "signalutils.h"

#include <sigc++/sigc++.h>
#include <glibmm/ustring.h>
#include <memory>

namespace Gtk
{
class CheckButton;
class ColorButton;
class ComboBox;
class Dialog;
class Entry;
class FontButton;
class Widget;
class Window;
}

namespace Gnome { namespace Conf { class Value; } }


namespace Regexxer
{

class PrefDialog : public sigc::trackable
{
public:
  explicit PrefDialog(Gtk::Window& parent);
  virtual ~PrefDialog();

  Gtk::Dialog* get_dialog() { return dialog_.get(); }

private:
  std::auto_ptr<Gtk::Dialog>  dialog_;
  Gtk::FontButton*            button_textview_font_;
  Gtk::ColorButton*           button_match_color_;
  Gtk::ColorButton*           button_current_color_;
  Gtk::ComboBox*              combo_toolbar_style_;
  Gtk::Entry*                 entry_fallback_;
  Util::AutoConnection        conn_toolbar_style_;
  bool                        entry_fallback_changed_;

  void load_xml();
  void connect_signals();

  void on_response(int response_id);

  void on_conf_value_changed(const Glib::ustring& key, const Gnome::Conf::Value& value);
  void initialize_configuration();

  void on_textview_font_set();
  void on_match_color_set();
  void on_current_color_set();
  void on_option_toolbar_style_changed();
  void on_entry_fallback_changed();
  void on_entry_fallback_activate();
};

} // namespace Regexxer

#endif /* REGEXXER_PREFDIALOG_H_INCLUDED */

