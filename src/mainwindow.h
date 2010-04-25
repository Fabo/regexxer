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

#ifndef REGEXXER_MAINWINDOW_H_INCLUDED
#define REGEXXER_MAINWINDOW_H_INCLUDED

#include "controller.h"
#include "filebuffer.h"
#include "sharedptr.h"
#include "completionstack.h"

#include <sigc++/sigc++.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <list>
#include <memory>
#include <vector>

namespace Gtk
{
class Button;
class CheckButton;
class Dialog;
class Entry;
class FileChooser;
class Toolbar;
class Window;
class ComboBoxEntry;
class ComboBoxEntryText;
class VBox;
class ScrolledWindow;
class Table;
class EntryCompletion;
}

namespace gtksourceview
{
class SourceView;
}

namespace Gnome { namespace Conf { class Value; } }

namespace Regexxer
{

class FileTree;
class PrefDialog;
class StatusLine;
struct FileInfo;

struct InitState
{
  std::vector<std::string>  folder;
  Glib::ustring             pattern;
  Glib::ustring             regex;
  Glib::ustring             substitution;
  bool                      no_recursive;
  bool                      hidden;
  bool                      no_global;
  bool                      ignorecase;
  bool                      feedback;
  bool                      no_autorun;

  InitState();
  ~InitState();
};

class MainWindow : public sigc::trackable
{
public:
  MainWindow();
  virtual ~MainWindow();

  void initialize(const InitState& init);
  Gtk::Window* get_window() { return window_.get(); }

private:
  class BusyAction;

  std::auto_ptr<Gtk::Window>  window_;
  Controller                  controller_;
  
  Gtk::VBox*                  vbox_main_;

  Gtk::Toolbar*               toolbar_;

  Gtk::Table*                 table_file_;
  Gtk::FileChooser*           button_folder_;
  
  Gtk::ComboBoxEntryText*     combo_entry_pattern_;
  
  Gtk::CheckButton*           button_recursive_;
  Gtk::CheckButton*           button_hidden_;

  Gtk::ComboBoxEntry*         comboboxentry_regex_;
  Gtk::Entry*                 entry_regex_;
  CompletionStack             entry_regex_completion_stack_;
  Glib::RefPtr<Gtk::EntryCompletion> entry_regex_completion_;
  
  Gtk::ComboBoxEntry*         comboboxentry_substitution_;
  Gtk::Entry*                 entry_substitution_;
  CompletionStack             entry_substitution_completion_stack_;
  Glib::RefPtr<Gtk::EntryCompletion> entry_substitution_completion_;
  
  Gtk::CheckButton*           button_multiple_;
  Gtk::CheckButton*           button_caseless_;

  FileTree*                   filetree_;
  Gtk::ScrolledWindow*        scrollwin_filetree_;
  Gtk::ScrolledWindow*        scrollwin_textview_;
  gtksourceview::SourceView*  textview_;
  Gtk::Entry*                 entry_preview_;

  StatusLine*                 statusline_;

  bool                        busy_action_running_;
  bool                        busy_action_cancel_;
  unsigned int                busy_action_iteration_;

  UndoStackPtr                undo_stack_;

  std::list<sigc::connection> buffer_connections_;

  std::auto_ptr<Gtk::Dialog>  about_dialog_;
  std::auto_ptr<PrefDialog>   pref_dialog_;

  void load_xml();
  void connect_signals();
  bool autorun_idle();

  void on_hide();
  void on_style_changed(const Glib::RefPtr<Gtk::Style>& previous_style);
  bool on_delete_event(GdkEventAny* event);

  void on_cut();
  void on_copy();
  void on_paste();
  void on_erase();

  void on_quit();
  bool confirm_quit_request();

  void on_find_files();
  void on_exec_search();
  bool after_exec_search();

  void on_filetree_switch_buffer(Util::SharedPtr<FileInfo> fileinfo, int file_index);
  void on_filetree_file_count_changed();
  void on_filetree_match_count_changed();
  void on_filetree_modified_count_changed();

  void on_bound_state_changed();
  void on_buffer_modified_changed();

  void on_go_next_file(bool move_forward);
  void on_go_next(bool move_forward);
  void on_replace();
  void on_replace_file();
  void on_replace_all();

  void on_save_file();
  void on_save_all();

  void on_undo_stack_push(UndoActionPtr action);
  void on_undo();
  void undo_stack_clear();

  void on_entry_pattern_changed();
  void update_preview();
  void set_title_filename(const std::string& filename);

  void busy_action_enter();
  void busy_action_leave();
  bool on_busy_action_pulse();
  void on_busy_action_cancel();

  void on_about();
  void on_about_dialog_response(int);

  void on_preferences();
  void on_pref_dialog_hide();

  void on_conf_value_changed(const Glib::ustring& key, const Gnome::Conf::Value& value);
};

} // namespace Regexxer

#endif /* REGEXXER_MAINWINDOW_H_INCLUDED */
