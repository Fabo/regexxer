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

#include "configdata.h"

#include <gtkmm/dialog.h>
#include <gtkmm/toolbar.h>

namespace Gtk
{
class Box;
class Entry;
class Label;
class RadioButton;
}


namespace Regexxer
{

class PrefDialog : public Gtk::Dialog
{
public:
  explicit PrefDialog(Gtk::Window& parent);
  virtual ~PrefDialog();

  void set_pref_menutool_mode(MenuToolMode menutool_mode);
  void set_pref_toolbar_style(Gtk::ToolbarStyle toolbar_style);
  void set_pref_fallback_encoding(const std::string& fallback_encoding);

  SigC::Signal1<void,MenuToolMode>        signal_pref_menutool_mode_changed;
  SigC::Signal1<void,Gtk::ToolbarStyle>   signal_pref_toolbar_style_changed;
  SigC::Signal1<void,const std::string&>  signal_pref_fallback_encoding_changed;

protected:
  virtual void on_response(int response_id);

private:
  Gtk::RadioButton*   button_menu_and_tool_;
  Gtk::RadioButton*   button_menu_only_;
  Gtk::RadioButton*   button_tool_only_;

  Gtk::Label*         label_toolbar_;
  Gtk::Box*           box_toolbar_;

  Gtk::RadioButton*   button_icons_;
  Gtk::RadioButton*   button_text_;
  Gtk::RadioButton*   button_both_;
  Gtk::RadioButton*   button_both_horiz_;

  Gtk::Entry*         entry_fallback_;

  MenuToolMode        current_menutool_mode_;
  Gtk::ToolbarStyle   current_toolbar_style_;

  Gtk::Widget* create_page_look();
  Gtk::Widget* create_page_file();

  void on_radio_menutool_mode_toggled();
  void on_radio_toolbar_style_toggled();
  void on_entry_fallback_activate();
};

} // namespace Regexxer

#endif /* REGEXXER_PREFDIALOG_H_INCLUDED */

