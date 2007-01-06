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

#ifndef REGEXXER_CONTROLLER_H_INCLUDED
#define REGEXXER_CONTROLLER_H_INCLUDED

#include <sigc++/sigc++.h>
#include <glibmm/refptr.h>


namespace Gtk
{
class MenuBar;
class Toolbar;
class Widget;
}

namespace Gnome { namespace Glade { class Xml; } }


namespace Regexxer
{

class ControlItem : public sigc::trackable
{
public:
  explicit ControlItem(bool enable = false);
  virtual ~ControlItem();

  void activate();
  sigc::slot<void> slot();

  void connect(const sigc::slot<void>& slot_activated);

  void add_widget(Gtk::Widget& widget);
  void add_widgets(const Glib::RefPtr<Gnome::Glade::Xml>& xml,
                   const char* menuitem_name, const char* button_name);

  void set_enabled(bool enable);
  void set_group_enabled(bool enable);
  bool is_enabled() const;

private:
  sigc::signal<void>      signal_activate_;
  sigc::signal<void,bool> signal_set_sensitive_;
  bool                    enabled_;
  bool                    group_enabled_;

  ControlItem(const ControlItem&);
  ControlItem& operator=(const ControlItem&);
};


class ControlGroup
{
public:
  explicit ControlGroup(bool enable = false);
  ~ControlGroup();

  void add(ControlItem& control);
  void set_enabled(bool enable);

private:
  sigc::signal<void,bool> signal_set_enabled_;
  bool                    enabled_;

  ControlGroup(const ControlGroup&);
  ControlGroup& operator=(const ControlGroup&);
};


class Controller
{
public:
  Controller();
  virtual ~Controller();

  // Group for all controls that could change matches
  // or require match information to operate.
  ControlGroup  match_actions;
  ControlGroup  edit_actions;

  ControlItem   save_file;
  ControlItem   save_all;
  ControlItem   undo;
  ControlItem   preferences;
  ControlItem   quit;
  ControlItem   about;

  ControlItem   find_files;
  ControlItem   find_matches;

  ControlItem   next_file;
  ControlItem   prev_file;
  ControlItem   next_match;
  ControlItem   prev_match;

  ControlItem   replace;
  ControlItem   replace_file;
  ControlItem   replace_all;

  ControlItem   cut;
  ControlItem   copy;
  ControlItem   paste;
  ControlItem   erase;

  void load_xml(const Glib::RefPtr<Gnome::Glade::Xml>& xml);

private:
  Controller(const Controller&);
  Controller& operator=(const Controller&);
};

} // namespace Regexxer

#endif /* REGEXXER_CONTROLLER_H_INCLUDED */
