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

#ifndef REGEXXER_CONTROLLER_H_INCLUDED
#define REGEXXER_CONTROLLER_H_INCLUDED

#include <sigc++/sigc++.h>


namespace Gtk
{
class MenuBar;
class Toolbar;
class Widget;
}


namespace Regexxer
{

class ControlItem : public SigC::Object
{
public:
  explicit ControlItem(bool enable = false);
  virtual ~ControlItem();

  void activate();
  SigC::Slot0<void> slot();

  void connect(const SigC::Slot0<void>& slot_activated);

  void add_widget(Gtk::Widget& widget);

  void set_enabled(bool enable);
  void set_group_enabled(bool enable);
  bool is_enabled() const;

private:
  SigC::Signal0<void>       signal_activate_;
  SigC::Signal1<void,bool>  signal_set_sensitive_;
  bool                      enabled_;
  bool                      group_enabled_;
};

class ControlGroup
{
public:
  explicit ControlGroup(bool enable = false);
  ~ControlGroup();

  void add(ControlItem& control);
  void set_enabled(bool enable);

private:
  SigC::Signal1<void,bool>  signal_set_enabled_;
  bool                      enabled_;

  ControlGroup(const ControlGroup&);
  ControlGroup& operator=(const ControlGroup&);
};


class Controller
{
public:
  Controller();
  virtual ~Controller();

  ControlItem   save_file;
  ControlItem   save_all;
  ControlItem   preferences;
  ControlItem   quit;

  // Group for all controls that could change matches
  // or require match information to operate.
  ControlGroup  match_actions;

  ControlItem   find_files;
  ControlItem   find_matches;

  ControlItem   next_file;
  ControlItem   prev_file;
  ControlItem   next_match;
  ControlItem   prev_match;

  ControlItem   replace;
  ControlItem   replace_file;
  ControlItem   replace_all;

  Gtk::MenuBar* create_menubar();
  Gtk::Toolbar* create_toolbar();
  Gtk::Widget*  create_action_area();

private:
  Controller(const Controller&);
  Controller& operator=(const Controller&);
};

} // namespace Regexxer

#endif /* REGEXXER_CONTROLLER_H_INCLUDED */

