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

#ifndef REGEXXER_STATUSLINE_H_INCLUDED
#define REGEXXER_STATUSLINE_H_INCLUDED

#include <gtkmm/box.h>
#include <string>

namespace Gtk
{
class Button;
class ProgressBar;
class Statusbar;
}


namespace Regexxer
{

class CounterBox;

class StatusLine : public Gtk::HBox
{
public:
  StatusLine();
  virtual ~StatusLine();

  void set_file_index(int file_index);
  void set_file_count(int file_count);

  void set_match_index(int match_index);
  void set_match_count(int match_count);

  void set_file_encoding(const std::string& file_encoding);

  void pulse_start();
  void pulse();
  void pulse_stop();

  SigC::Signal0<void> signal_cancel_clicked;

protected:
  virtual void on_hierarchy_changed(Gtk::Widget* previous_toplevel);

private:
  Gtk::Button*        cancel_button_;
  Gtk::ProgressBar*   progressbar_;
  CounterBox*         file_counter_;
  CounterBox*         match_counter_;
  Gtk::Statusbar*     statusbar_;

  void on_button_style_changed(const Glib::RefPtr<Gtk::Style>& previous_style);
};

} // namespace Regexxer

#endif /* REGEXXER_STATUSLINE_H_INCLUDED */

