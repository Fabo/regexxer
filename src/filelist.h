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

#ifndef REGEXXER_FILELIST_H_INCLUDED
#define REGEXXER_FILELIST_H_INCLUDED

#include <gdkmm/color.h>
#include <gtkmm/treepath.h>
#include <gtkmm/treeview.h>

#include "filebuffer.h"
#include "fileio.h"

namespace Gtk  { class ListStore; }
namespace Pcre { class Pattern;   }


namespace Regexxer
{

class FileList : public Gtk::TreeView
{
public:
  FileList();
  virtual ~FileList();

  void find_files(const Glib::ustring& dirname,
                  const Glib::ustring& pattern,
                  bool recursive, bool hidden);
  void stop_find_files();

  void save_current_file();
  void save_all_files();

  void select_first_file();
  bool select_next_file(bool move_forward);

  BoundState get_bound_state();

  void find_matches(Pcre::Pattern& pattern, bool multiple);
  long get_match_count() const;
  void replace_all_matches(const Glib::ustring& substitution);

  int get_modified_count() const;

  SigC::Signal1<void,FileInfoPtr> signal_switch_buffer;
  SigC::Signal0<void>             signal_bound_state_changed;
  SigC::Signal0<void>             signal_match_count_changed;
  SigC::Signal0<void>             signal_modified_count_changed;

protected:
  virtual void on_style_changed(const Glib::RefPtr<Gtk::Style>& previous_style);

private:
  struct FindData;

  Glib::RefPtr<Gtk::ListStore>  liststore_;
  Gdk::Color                    color_modified_;
  Gdk::Color                    color_load_failed_;
  bool                          find_running_;
  bool                          find_stop_;
  long                          sum_matches_;
  int                           modified_count_;
  SigC::Connection              conn_match_count_;
  SigC::Connection              conn_modified_changed_;
  Gtk::TreePath                 path_match_first_;
  Gtk::TreePath                 path_match_last_;

  void cell_data_func(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& iter);

  void find_recursively(const std::string& dirname, FindData& find_data);
  bool find_check_file(const std::string& basename, const std::string& fullname, FindData& find_data);

  void on_selection_changed();
  void on_buffer_match_count_changed(int match_count);
  void on_buffer_modified_changed();
};

} // namespace Regexxer

#endif /* REGEXXER_FILELIST_H_INCLUDED */

