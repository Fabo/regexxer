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

#include <gdkmm/color.h>
#include <gtkmm/dialog.h>
#include <gtkmm/toolbar.h>

namespace Gtk
{
class Entry;
class OptionMenu;
}


namespace Regexxer
{

class ColorSelectionButton;
class FontSelectionButton;

class PrefDialog : public Gtk::Dialog
{
public:
  explicit PrefDialog(Gtk::Window& parent);
  virtual ~PrefDialog();

  void set_pref_textview_font(const Pango::FontDescription& textview_font);
  void set_pref_match_color(const Gdk::Color& match_color);
  void set_pref_current_color(const Gdk::Color& current_color);
  void set_pref_toolbar_style(Gtk::ToolbarStyle toolbar_style);
  void set_pref_fallback_encoding(const std::string& fallback_encoding);

  SigC::Signal1<void,const Pango::FontDescription&> signal_pref_textview_font_changed;
  SigC::Signal1<void,const Gdk::Color&>             signal_pref_match_color_changed;
  SigC::Signal1<void,const Gdk::Color&>             signal_pref_current_color_changed;
  SigC::Signal1<void,Gtk::ToolbarStyle>             signal_pref_toolbar_style_changed;
  SigC::Signal1<void,const std::string&>            signal_pref_fallback_encoding_changed;

protected:
  virtual void on_response(int response_id);

private:
  FontSelectionButton*  button_textview_font_;
  ColorSelectionButton* button_match_color_;
  ColorSelectionButton* button_current_color_;
  Gtk::OptionMenu*      option_toolbar_style_;
  Gtk::Entry*           entry_fallback_;

  Gtk::Widget* create_page_look();
  Gtk::Widget* create_page_file();

  void on_textview_font_selected();
  void on_match_color_selected();
  void on_current_color_selected();
  void on_option_toolbar_style_changed();
  void on_entry_fallback_activate();
};

} // namespace Regexxer

#endif /* REGEXXER_PREFDIALOG_H_INCLUDED */

