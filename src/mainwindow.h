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

#ifndef REGEXXER_MAINWINDOW_H_INCLUDED
#define REGEXXER_MAINWINDOW_H_INCLUDED

#include "controller.h"
#include "filebuffer.h"
#include "sharedptr.h"

#include <gtkmm/tooltips.h>
#include <gtkmm/window.h>

#include <list>
#include <memory>

namespace Gtk
{
class Button;
class CheckButton;
class Entry;
class HandleBox;
class TextBuffer;
class TextView;
}


namespace Regexxer
{

class AboutDialog;
class FileTree;
class PrefDialog;
class StatusLine;
struct FileInfo;

class MainWindow : public Gtk::Window
{
public:
  MainWindow();
  virtual ~MainWindow();

protected:
  virtual void on_hide();
  virtual void on_style_changed(const Glib::RefPtr<Gtk::Style>& previous_style);
  virtual bool on_delete_event(GdkEventAny* event);

private:
  class BusyAction;

  Controller        controller_;
  Gtk::Tooltips     tooltips_;

  Gtk::Toolbar*     toolbar_;

  Gtk::Entry*       entry_folder_;
  Gtk::Entry*       entry_pattern_;
  Gtk::CheckButton* button_recursive_;
  Gtk::CheckButton* button_hidden_;

  Gtk::Entry*       entry_regex_;
  Gtk::Entry*       entry_substitution_;
  Gtk::CheckButton* button_multiple_;
  Gtk::CheckButton* button_caseless_;

  FileTree*         filetree_;
  Gtk::TextView*    textview_;
  Gtk::Entry*       entry_preview_;

  StatusLine*       statusline_;

  bool              busy_action_running_;
  bool              busy_action_cancel_;
  unsigned int      busy_action_iteration_;

  UndoStackPtr      undo_stack_;

  std::list<sigc::connection> buffer_connections_;

  std::auto_ptr<AboutDialog>  about_dialog_;
  std::auto_ptr<PrefDialog>   pref_dialog_;

  Gtk::Widget* create_main_vbox();
  Gtk::Widget* create_left_pane();
  Gtk::Widget* create_right_pane();

  void on_quit();
  bool confirm_quit_request();

  void on_select_folder();
  void on_find_files();
  void on_exec_search();
  bool after_exec_search();

  void on_filetree_switch_buffer(Util::SharedPtr<FileInfo> fileinfo, int file_index);
  void on_filetree_file_count_changed();
  void on_filetree_match_count_changed();
  void on_filetree_modified_count_changed();
  void on_filetree_bound_state_changed();

  void on_buffer_match_count_changed(int match_count);
  void on_buffer_modified_changed();
  void on_buffer_bound_state_changed(BoundState bound);

  void on_go_next_file(bool move_forward);
  void on_go_next(bool move_forward);
  void on_replace();
  void on_replace_file();
  void on_replace_all();

  void on_save_file();
  void on_save_all();

  void on_undo_stack_push(UndoActionPtr action);
  void on_undo();

  void on_entry_pattern_changed();
  void update_preview();
  void set_title_filename(const Glib::ustring& filename = Glib::ustring());

  void busy_action_enter();
  void busy_action_leave();
  bool on_busy_action_pulse();
  void on_busy_action_cancel();

  void on_info();
  void on_about_dialog_hide();

  void on_preferences();
  void on_pref_dialog_hide();

  void load_configuration();
  void save_configuration();

  // Work-around for silly, stupid, and annoying gcc 2.95.x.
  friend class MainWindow::BusyAction;
};

} // namespace Regexxer

#endif /* REGEXXER_MAINWINDOW_H_INCLUDED */

