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

#ifndef REGEXXER_FILETREE_H_INCLUDED
#define REGEXXER_FILETREE_H_INCLUDED

#include "filebuffer.h"
#include "fileio.h"
#include "undostack.h"

#include <gdkmm/color.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/treepath.h>
#include <gtkmm/treeview.h>
#include <list>
#include <stack>

namespace Gtk  { class TreeStore; }
namespace Pcre { class Pattern;   }


namespace Regexxer
{

class FileTree : public Gtk::TreeView
{
public:
  class Error; // exception class

  FileTree();
  virtual ~FileTree();

  void find_files(const std::string& dirname, Pcre::Pattern& pattern,
                  bool recursive, bool hidden);

  int  get_file_count() const;

  void save_current_file();
  void save_all_files();

  void select_first_file();
  bool select_next_file(bool move_forward);

  BoundState get_bound_state();

  void find_matches(Pcre::Pattern& pattern, bool multiple);
  long get_match_count() const;
  void replace_all_matches(const Glib::ustring& substitution);

  int get_modified_count() const;

  void set_fallback_encoding(const std::string& fallback_encoding);
  std::string get_fallback_encoding() const;

  SigC::Signal2<void,FileInfoPtr,int> signal_switch_buffer;
  SigC::Signal0<void>                 signal_bound_state_changed;
  SigC::Signal0<void>                 signal_file_count_changed;
  SigC::Signal0<void>                 signal_match_count_changed;
  SigC::Signal0<void>                 signal_modified_count_changed;
  SigC::Signal0<bool>                 signal_pulse;
  SigC::Signal1<void,UndoActionPtr>   signal_undo_stack_push;

protected:
  virtual void on_style_changed(const Glib::RefPtr<Gtk::Style>& previous_style);

private:
  class  TreeRowRef;
  class  MessageList;
  class  ScopedBlockSorting;
  class  BufferActionShell;
  struct FindData;
  struct FindMatchesData;
  struct ReplaceMatchesData;

  typedef Util::SharedPtr<TreeRowRef>        TreeRowRefPtr;
  typedef Util::SharedPtr<BufferActionShell> BufferActionShellPtr;

  Glib::RefPtr<Gtk::TreeStore>  treestore_;

  Glib::RefPtr<Gdk::Pixbuf>     pixbuf_directory_;
  Glib::RefPtr<Gdk::Pixbuf>     pixbuf_file_;
  Glib::RefPtr<Gdk::Pixbuf>     pixbuf_load_failed_;

  Gdk::Color                    color_modified_;
  Gdk::Color                    color_load_failed_;

  TreeRowRefPtr                 last_selected_rowref_;
  FileInfoPtr                   last_selected_;

  DirInfo                       toplevel_;
  long                          sum_matches_;

  SigC::Connection              conn_match_count_;
  SigC::Connection              conn_modified_changed_;
  SigC::Connection              conn_undo_stack_push_;

  Gtk::TreePath                 path_match_first_;
  Gtk::TreePath                 path_match_last_;

  std::string                   fallback_encoding_;
  Glib::RefPtr<Gdk::Pixbuf>     error_pixbuf_;

  void icon_cell_data_func(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& iter);
  void text_cell_data_func(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& iter);

  static bool select_func(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreePath& path,
                          bool currently_selected);

  void find_recursively(const std::string& dirname, FindData& find_data);
  bool find_check_file(const std::string& basename, const std::string& fullname, FindData& find_data);
  void find_fill_dirstack(FindData& find_data);
  void find_increment_file_count(FindData& find_data, int file_count);

  bool save_file_at_iter(const Gtk::TreeModel::iterator& iter,
                         Util::SharedPtr<MessageList>* error_list);

  bool find_matches_at_iter(const Gtk::TreeModel::iterator& iter, FindMatchesData* find_data);

  bool replace_matches_at_iter(const Gtk::TreeModel::iterator& iter,
                               ReplaceMatchesData* replace_data);

  bool next_match_file(Gtk::TreeModel::iterator& iter, std::stack<Gtk::TreePath>* collapse_stack = 0);
  bool prev_match_file(Gtk::TreeModel::iterator& iter, std::stack<Gtk::TreePath>* collapse_stack = 0);

  void expand_and_select(const Gtk::TreePath& path);

  void on_treestore_sort_column_changed();
  void on_selection_changed();
  void on_buffer_match_count_changed(int match_count);
  void on_buffer_modified_changed();
  void on_buffer_undo_stack_push(UndoActionPtr undo_action);

  int calculate_file_index(const Gtk::TreeModel::iterator& pos);

  void propagate_match_count_change(const Gtk::TreeModel::iterator& pos, int difference);
  void propagate_modified_change(const Gtk::TreeModel::iterator& pos, bool modified);

  void load_file_with_fallback(const Gtk::TreeModel::iterator& iter, const FileInfoPtr& fileinfo);
  Glib::RefPtr<FileBuffer> create_error_message_buffer(const Glib::ustring& message);

  // Work-around for silly, stupid, and annoying gcc 2.95.x.
  friend class FileTree::ScopedBlockSorting;
  friend class FileTree::BufferActionShell;
};


class FileTree::Error
{
public:
  explicit Error(const Util::SharedPtr<FileTree::MessageList>& error_list);
  virtual ~Error();

  Error(const FileTree::Error& other);
  FileTree::Error& operator=(const FileTree::Error& other);

  const std::list<Glib::ustring>& get_error_list() const;

private:
  Util::SharedPtr<FileTree::MessageList> error_list_;
};

} // namespace Regexxer

#endif /* REGEXXER_FILETREE_H_INCLUDED */

