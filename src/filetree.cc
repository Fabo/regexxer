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

#include "filetree.h"
#include "filetreeprivate.h"
#include "pcreshell.h"
#include "signalutils.h"
#include "stringutils.h"

#include <gtkmm/stock.h>

using namespace Regexxer::FileTreePrivate;


namespace Regexxer
{

/**** Regexxer::FileTree ***************************************************/

FileTree::FileTree()
:
  treestore_          (Gtk::TreeStore::create(filetree_columns())),
  color_modified_     ("red"),
  sum_matches_        (0),
  fallback_encoding_  ("ISO-8859-15")
{
  using namespace Gtk;

  set_model(treestore_);
  const FileTreeColumns& model_columns = filetree_columns();

  treestore_->set_default_sort_func(&default_sort_func);
  treestore_->set_sort_func(model_columns.collatekey.index(), &collatekey_sort_func);
  treestore_->set_sort_column_id(TreeStore::DEFAULT_SORT_COLUMN_ID, SORT_ASCENDING);

  treestore_->signal_sort_column_changed().connect(
      SigC::slot(*this, &FileTree::on_treestore_sort_column_changed));

  {
    Column *const column = new Column("File");
    append_column(*manage(column));

    CellRendererPixbuf *const cell_icon = new CellRendererPixbuf();
    column->pack_start(*manage(cell_icon), false);

    CellRendererText *const cell_filename = new CellRendererText();
    column->pack_start(*manage(cell_filename));

    column->add_attribute(cell_filename->property_text(), model_columns.filename);
    column->set_cell_data_func(*cell_icon,     SigC::slot(*this, &FileTree::icon_cell_data_func));
    column->set_cell_data_func(*cell_filename, SigC::slot(*this, &FileTree::text_cell_data_func));

    column->set_resizable(true);

    column->set_sort_column_id(model_columns.collatekey.index());
  }

  {
    Column *const column = new Column("#");
    append_column(*manage(column));

    CellRendererText *const cell_matchcount = new CellRendererText();
    column->pack_start(*manage(cell_matchcount));

    column->add_attribute(cell_matchcount->property_text(), model_columns.matchcount);
    column->set_cell_data_func(*cell_matchcount, SigC::slot(*this, &FileTree::text_cell_data_func));

    column->set_alignment(1.0);
    cell_matchcount->property_xalign() = 1.0;

    column->set_sort_column_id(model_columns.matchcount.index());
  }

  set_search_column(model_columns.filename.index());

  const Glib::RefPtr<Gtk::TreeSelection> selection = get_selection();

  selection->set_select_function(&FileTree::select_func);
  selection->signal_changed().connect(SigC::slot(*this, &FileTree::on_selection_changed));
}

FileTree::~FileTree()
{}

void FileTree::find_files(const std::string& dirname, Pcre::Pattern& pattern,
                          bool recursive, bool hidden)
{
  FindData find_data (pattern, recursive, hidden);

  const bool modified_count_changed = (toplevel_.modified_count != 0);

  get_selection()->unselect_all(); // workaround for GTK+ <= 2.0.6 (#94868)
  treestore_->clear();

  // Don't keep the pixbuf around if we don't need it.
  // It's recreated on demand if necessary.
  error_pixbuf_.clear();

  toplevel_.file_count     = 0;
  toplevel_.modified_count = 0;
  sum_matches_ = 0;

  signal_bound_state_changed(); // emit
  signal_match_count_changed(); // emit

  if(modified_count_changed)
    signal_modified_count_changed(); // emit

  try
  {
    find_recursively(dirname, find_data);
  }
  catch(const Glib::FileError& error)
  {
    find_data.error_list->push_back(error.what()); // collect errors but don't fail
  }

  signal_bound_state_changed(); // emit

  if(!find_data.error_list->empty())
    throw Error(find_data.error_list);
}

int FileTree::get_file_count() const
{
  g_return_val_if_fail(toplevel_.file_count >= 0, 0);
  return toplevel_.file_count;
}

void FileTree::save_current_file()
{
  if(const Gtk::TreeModel::iterator selected = get_selection()->get_selected())
  {
    Util::SharedPtr<MessageList> error_list;

    {
      Util::ScopedBlock block (conn_modified_changed_);
      save_file_at_iter(selected, &error_list);
    }

    if(error_list)
      throw Error(error_list);
  }
}

void FileTree::save_all_files()
{
  Util::SharedPtr<MessageList> error_list;

  {
    Util::ScopedBlock block (conn_modified_changed_);
    treestore_->foreach(SigC::bind(SigC::slot(*this, &FileTree::save_file_at_iter), &error_list));
  }

  if(error_list)
    throw Error(error_list);
}

void FileTree::select_first_file()
{
  if(sum_matches_ > 0)
    expand_and_select(path_match_first_);
}

bool FileTree::select_next_file(bool move_forward)
{
  if(Gtk::TreeModel::iterator iter = get_selection()->get_selected())
  {
    std::stack<Gtk::TreePath> collapse_stack;

    if((move_forward) ? next_match_file(iter, &collapse_stack)
                      : prev_match_file(iter, &collapse_stack))
    {
      for(; !collapse_stack.empty(); collapse_stack.pop())
        collapse_row(collapse_stack.top());

      expand_and_select(Gtk::TreePath(iter));

      return true;
    }
  }

  return false;
}

BoundState FileTree::get_bound_state()
{
  BoundState bound = BOUND_FIRST | BOUND_LAST;

  if(sum_matches_ > 0)
  {
    if(const Gtk::TreeModel::iterator iter = get_selection()->get_selected())
    {
      Gtk::TreePath path (iter);

      if(path > path_match_first_)
        bound &= ~BOUND_FIRST;

      if(path < path_match_last_)
        bound &= ~BOUND_LAST;
    }
  }

  return bound;
}

void FileTree::find_matches(Pcre::Pattern& pattern, bool multiple)
{
  {
    Util::ScopedBlock  block_conn (conn_match_count_);
    ScopedBlockSorting block_sort (*this);
    FindMatchesData find_data (pattern, multiple);

    treestore_->foreach(SigC::bind(SigC::slot(*this, &FileTree::find_matches_at_iter), &find_data));
  }

  signal_bound_state_changed(); // emit
}

long FileTree::get_match_count() const
{
  g_return_val_if_fail(sum_matches_ >= 0, 0);
  return sum_matches_;
}

void FileTree::replace_all_matches(const Glib::ustring& substitution)
{
  {
    Util::ScopedBlock block_match_count      (conn_match_count_);
    Util::ScopedBlock block_modified_changed (conn_modified_changed_);
    Util::ScopedBlock block_undo_stack_push  (conn_undo_stack_push_);
    ScopedBlockSorting block_sort (*this);
    ReplaceMatchesData replace_data (*this, substitution);

    treestore_->foreach(SigC::bind(
        SigC::slot(*this, &FileTree::replace_matches_at_iter),
        &replace_data));

    signal_undo_stack_push(replace_data.undo_stack); // emit
  }

  signal_bound_state_changed(); // emit
}

int FileTree::get_modified_count() const
{
  g_return_val_if_fail(toplevel_.modified_count >= 0, 0);
  return toplevel_.modified_count;
}

void FileTree::set_fallback_encoding(const std::string& fallback_encoding)
{
  fallback_encoding_ = fallback_encoding;
}

std::string FileTree::get_fallback_encoding() const
{
  return fallback_encoding_;
}

/**** Regexxer::FileTree -- protected **************************************/

void FileTree::on_style_changed(const Glib::RefPtr<Gtk::Style>& previous_style)
{
  const Glib::ustring detail = "regexxer-filetree";

  pixbuf_directory_   = render_icon(Gtk::Stock::OPEN,          Gtk::ICON_SIZE_MENU, detail);
  pixbuf_file_        = render_icon(Gtk::Stock::NEW,           Gtk::ICON_SIZE_MENU, detail);
  pixbuf_load_failed_ = render_icon(Gtk::Stock::MISSING_IMAGE, Gtk::ICON_SIZE_MENU, detail);

  color_load_failed_ = get_style()->get_text(Gtk::STATE_INSENSITIVE);

  error_pixbuf_.clear();

  Gtk::TreeView::on_style_changed(previous_style);
}

/**** Regexxer::FileTree -- private ****************************************/

void FileTree::icon_cell_data_func(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& iter)
{
  Gtk::CellRendererPixbuf& renderer = dynamic_cast<Gtk::CellRendererPixbuf&>(*cell);
  const FileInfoBasePtr    infobase = (*iter)[filetree_columns().fileinfo];

  if(const FileInfoPtr fileinfo = shared_dynamic_cast<FileInfo>(infobase))
  {
    renderer.property_pixbuf() = (fileinfo->load_failed) ? pixbuf_load_failed_ : pixbuf_file_;
  }
  else if(shared_dynamic_cast<DirInfo>(infobase))
  {
    renderer.property_pixbuf() = pixbuf_directory_;
  }
  else
  {
    renderer.property_pixbuf().reset_value();
  }
}

void FileTree::text_cell_data_func(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& iter)
{
  Gtk::CellRendererText& renderer = dynamic_cast<Gtk::CellRendererText&>(*cell);
  const FileInfoBasePtr  infobase = (*iter)[filetree_columns().fileinfo];

  Gdk::Color* color = 0;

  if(const FileInfoPtr fileinfo = shared_dynamic_cast<FileInfo>(infobase))
  {
    if(fileinfo->load_failed)
      color = &color_load_failed_;
    else if(fileinfo->buffer && fileinfo->buffer->get_modified())
      color = &color_modified_;
  }
  else if(const DirInfoPtr dirinfo = shared_dynamic_cast<DirInfo>(infobase))
  {
    if(dirinfo->modified_count > 0)
      color = &color_modified_;
  }

  if(color)
    renderer.property_foreground_gdk() = *color;
  else
    renderer.property_foreground_gdk().reset_value();
}

// static
bool FileTree::select_func(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreePath& path, bool)
{
  // Don't allow selection of directory nodes.
  return get_fileinfo_from_iter(model->get_iter(path));
}

void FileTree::find_recursively(const std::string& dirname, FindData& find_data)
{
  using namespace Glib;

  int file_count = 0;
  Dir dir (dirname);

  for(Dir::iterator pos = dir.begin(); pos != dir.end(); ++pos)
  {
    if(signal_pulse()) // emit
      break;

    const std::string basename (*pos);

    if(!find_data.hidden && *basename.begin() == '.')
      continue;

    const std::string fullname (build_filename(dirname, basename));

    try
    {
      if(find_check_file(basename, fullname, find_data)) // file added?
        ++file_count;
    }
    catch(const Glib::FileError& error)
    {
      // Collect errors but don't interrupt the search.
      find_data.error_list->push_back(error.what());
    }
    catch(const Glib::ConvertError& error) // unlikely due to use of our own fallback conversion
    {
      // Don't use Glib::locale_to_utf8() because we already
      // tried that in Util::filename_to_utf8_fallback().
      //
      const Glib::ustring name = Util::convert_to_ascii(fullname);
      const Glib::ustring what = error.what();

      g_warning("Eeeek, can't convert filename `%s' to UTF-8: %s", name.c_str(), what.c_str());
    }
  }

  find_increment_file_count(find_data, file_count);
}

bool FileTree::find_check_file(const std::string& basename, const std::string& fullname,
                               FindData& find_data)
{
  using namespace Glib;

  if(file_test(fullname, FILE_TEST_IS_SYMLINK))
    return false;

  if(find_data.recursive && file_test(fullname, FILE_TEST_IS_DIR))
  {
    // Put the directory name on the stack instead of creating a new node
    // immediately.  The corresponding node will be created on demand if
    // there's actually a matching file in the directory or one of its
    // subdirectories.
    //
    ScopedPushDir pushdir (find_data.dirstack, basename);
    find_recursively(fullname, find_data); // recurse
  }
  else if(file_test(fullname, FILE_TEST_IS_REGULAR))
  {
    const ustring basename_utf8 (Util::filename_to_utf8_fallback(basename));

    if(find_data.pattern.match(basename_utf8) > 0)
    {
      // Build the collate key with a leading '1' so that directories always
      // come first (they have a leading '0').  This is simpler and faster
      // than explicitely checking for directories in the sort function.
      std::string collate_key (1, '1');
      collate_key += basename_utf8.collate_key();

      Gtk::TreeModel::Row row;

      if(find_data.dirstack.empty())
      {
        row = *treestore_->prepend(); // new toplevel node
      }
      else
      {
        if(!find_data.dirstack.back().second)
          find_fill_dirstack(find_data); // build all directory nodes in the stack

        row = *treestore_->prepend(find_data.dirstack.back().second->children());
      }

      const FileTreeColumns& columns = filetree_columns();

      row[columns.filename]   = basename_utf8;
      row[columns.collatekey] = collate_key;
      row[columns.fileinfo]   = FileInfoBasePtr(new FileInfo(fullname));

      return true; // a file has been added
    }
  }

  return false;
}

void FileTree::find_fill_dirstack(FindData& find_data)
{
  const FileTreeColumns& columns = filetree_columns();

  const DirStack::iterator pend  = find_data.dirstack.end();
  DirStack::iterator       pprev = pend;

  for(DirStack::iterator pdir = find_data.dirstack.begin(); pdir != pend; pprev = pdir++)
  {
    if(pdir->second) // node already created
      continue;

    const Glib::ustring dirname (Util::filename_to_utf8_fallback(pdir->first));

    // Build the collate key with a leading '0' so that directories always
    // come first.  This is simpler and faster than explicitely checking for
    // directories in the sort function.
    std::string collate_key (1, '0');
    collate_key += dirname.collate_key();

    if(pprev == pend)
      pdir->second = treestore_->prepend(); // new toplevel node
    else
      pdir->second = treestore_->prepend(pprev->second->children());

    Gtk::TreeModel::Row row (*pdir->second);

    row[columns.filename]   = dirname;
    row[columns.collatekey] = collate_key;
    row[columns.fileinfo]   = FileInfoBasePtr(new DirInfo());
  }
}

void FileTree::find_increment_file_count(FindData& find_data, int file_count)
{
  if(file_count <= 0)
    return;

  const FileTreeColumns& columns = filetree_columns();

  DirStack::reverse_iterator       pdir = find_data.dirstack.rbegin();
  const DirStack::reverse_iterator pend = find_data.dirstack.rend();

  for(; pdir != pend; ++pdir)
  {
    const FileInfoBasePtr base = (*pdir->second)[columns.fileinfo];
    shared_polymorphic_cast<DirInfo>(base)->file_count += file_count;
  }

  toplevel_.file_count += file_count;
  signal_file_count_changed(); // emit
}

bool FileTree::save_file_at_iter(const Gtk::TreeModel::iterator& iter,
                                 Util::SharedPtr<MessageList>* error_list)
{
  const FileInfoPtr fileinfo = get_fileinfo_from_iter(iter);

  if(fileinfo && fileinfo->buffer && fileinfo->buffer->get_modified())
  {
    try
    {
      save_file(fileinfo);
    }
    catch(const Glib::Error& error)
    {
      if(!*error_list)
        error_list->reset(new MessageList());

      Glib::ustring message = "Failed to save file '";
      message += Util::filename_to_utf8_fallback(fileinfo->fullname);
      message += "': ";
      message += error.what();

      (*error_list)->push_back(message);
    }

    if(!fileinfo->buffer->get_modified())
      propagate_modified_change(iter, false);

    if(fileinfo != last_selected_ && fileinfo->buffer->is_freeable())
      fileinfo->buffer.clear(); // reduce memory footprint
  }

  return false;
}

bool FileTree::find_matches_at_iter(const Gtk::TreeModel::iterator& iter, FindMatchesData* find_data)
{
  if(signal_pulse()) // emit
    return true;

  if(const FileInfoPtr fileinfo = get_fileinfo_from_iter(iter))
  {
    if(!fileinfo->buffer)
      load_file_with_fallback(iter, fileinfo);

    if(fileinfo->load_failed)
      return false; // continue

    const Glib::RefPtr<FileBuffer> buffer = fileinfo->buffer;
    g_assert(buffer);

    Util::ScopedConnection conn (buffer->signal_pulse.connect(signal_pulse.slot()));

    const int old_match_count = buffer->get_match_count();
    const int new_match_count = buffer->find_matches(find_data->pattern, find_data->multiple);

    if(new_match_count > 0)
    {
      const Gtk::TreePath path (iter);

      if(!find_data->path_match_first_set)
      {
        path_match_first_ = path;
        find_data->path_match_first_set = true;
      }

      path_match_last_ = path;
    }

    if(new_match_count != old_match_count)
      propagate_match_count_change(iter, new_match_count - old_match_count);

    if(fileinfo != last_selected_ && buffer->is_freeable())
      fileinfo->buffer.clear(); // reduce memory footprint
  }

  return false;
}

bool FileTree::replace_matches_at_iter(const Gtk::TreeModel::iterator& iter,
                                       ReplaceMatchesData* replace_data)
{
  if(signal_pulse()) // emit
    return true;

  const FileInfoPtr fileinfo = get_fileinfo_from_iter(iter);

  if(fileinfo && fileinfo->buffer)
  {
    const Glib::RefPtr<FileBuffer> buffer = fileinfo->buffer;

    const int match_count = buffer->get_match_count();

    if(match_count > 0)
    {
      if(fileinfo != last_selected_)
        replace_data->row_reference.reset(new TreeRowRef(treestore_, Gtk::TreePath(iter)));
      else
        replace_data->row_reference = last_selected_rowref_;

      const bool was_modified = buffer->get_modified();

      {
        // Redirect the buffer's signal_undo_stack_push() in order to create
        // a single user action object for all replacements in all buffers.
        // Note that the caller must block conn_undo_stack_push_ to avoid
        // double notification.
        Util::ScopedConnection conn1 (buffer->signal_undo_stack_push.
                                      connect(replace_data->slot_undo_stack_push));

        Util::ScopedConnection conn2 (buffer->signal_pulse.connect(signal_pulse.slot()));

        buffer->replace_all_matches(replace_data->substitution);
      }

      const bool is_modified = buffer->get_modified();

      if(was_modified != is_modified)
        propagate_modified_change(iter, is_modified);

      g_return_val_if_fail(buffer->get_match_count() == 0, false);

      propagate_match_count_change(iter, -match_count);
    }
  }

  return false;
}

bool FileTree::next_match_file(Gtk::TreeModel::iterator& iter,
                               std::stack<Gtk::TreePath>* collapse_stack)
{
  g_return_val_if_fail(iter, false);

  const FileTreeColumns& columns = filetree_columns();
  Gtk::TreeModel::iterator parent = iter->parent();

  for(++iter;;)
  {
    if(iter)
    {
      if((*iter)[columns.matchcount] > 0)
      {
        if(const Gtk::TreeModel::Children& children = iter->children()) // directory?
        {
          parent = iter;
          iter = children.begin();
          continue;
        }

        return true;
      }
    }
    else if(parent)
    {
      iter = parent;
      parent = iter->parent();

      if(collapse_stack)
      {
        const Gtk::TreePath path (iter);

        if(row_expanded(path))
          collapse_stack->push(path);
      }
    }
    else
      break;

    ++iter;
  }

  return false;
}

bool FileTree::prev_match_file(Gtk::TreeModel::iterator& iter,
                               std::stack<Gtk::TreePath>* collapse_stack)
{
  g_return_val_if_fail(iter, false);

  const FileTreeColumns& columns = filetree_columns();
  Gtk::TreeModel::iterator parent = iter->parent();
  Gtk::TreePath path (iter);

  for(;;)
  {
    if(path.prev())
    {
      iter = treestore_->get_iter(path);

      if((*iter)[columns.matchcount] > 0)
      {
        if(const Gtk::TreeModel::Children& children = iter->children()) // directory?
        {
          parent = iter;
          path.append_index(children.size());
          continue;
        }

        return true;
      }
    }
    else if(parent)
    {
      path = Gtk::TreePath(parent);
      parent = parent->parent();

      if(collapse_stack && row_expanded(path))
        collapse_stack->push(path);
    }
    else
      break;
  }

  return false;
}

void FileTree::expand_and_select(const Gtk::TreePath& path)
{
  std::stack<Gtk::TreePath> parents;

  for(Gtk::TreePath parent (path); parent.up(); )
    parents.push(parent);

  for(; !parents.empty(); parents.pop())
    expand_row(parents.top(), false);

  get_selection()->select(path);
  scroll_to_row(path, 0.5);
}

void FileTree::on_treestore_sort_column_changed()
{
  const FileTreeColumns& columns = filetree_columns();

  if(sum_matches_ > 0)
  {
    Gtk::TreeModel::iterator first = treestore_->children().begin();

    while(first->children() && (*first)[columns.matchcount] > 0)
      first = first->children().begin();

    if((*first)[columns.matchcount] == 0)
    {
      const bool found_first = next_match_file(first);
      g_return_if_fail(found_first);
    }

    Gtk::TreeModel::iterator last = treestore_->children()[treestore_->children().size() - 1];

    while(last->children() && (*last)[columns.matchcount] > 0)
      last = last->children()[last->children().size() - 1];

    if((*last)[columns.matchcount] == 0)
    {
      const bool found_last = prev_match_file(last);
      g_return_if_fail(found_last);
    }

    path_match_first_ = Gtk::TreePath(first);
    path_match_last_  = Gtk::TreePath(last);

    signal_bound_state_changed(); // emit
  }
}

void FileTree::on_selection_changed()
{
  last_selected_rowref_.reset();

  FileInfoPtr fileinfo;
  int file_index = 0;

  const bool conn_match_count_blocked      = conn_match_count_.blocked();
  const bool conn_modified_changed_blocked = conn_modified_changed_.blocked();
  const bool conn_undo_stack_push_blocked  = conn_undo_stack_push_.blocked();

  conn_match_count_     .disconnect();
  conn_modified_changed_.disconnect();
  conn_undo_stack_push_ .disconnect();

  if(const Gtk::TreeModel::iterator iter = get_selection()->get_selected())
  {
    const FileTreeColumns& columns = filetree_columns();
    const FileInfoBasePtr base = (*iter)[columns.fileinfo];

    fileinfo   = shared_polymorphic_cast<FileInfo>(base);
    file_index = calculate_file_index(iter) + 1;

    if(!fileinfo->buffer)
      load_file_with_fallback(iter, fileinfo);

    if(!fileinfo->load_failed)
    {
      conn_match_count_ = fileinfo->buffer->signal_match_count_changed.
          connect(SigC::slot(*this, &FileTree::on_buffer_match_count_changed));

      conn_modified_changed_ = fileinfo->buffer->signal_modified_changed().
          connect(SigC::slot(*this, &FileTree::on_buffer_modified_changed));

      conn_undo_stack_push_ = fileinfo->buffer->signal_undo_stack_push.
          connect(SigC::slot(*this, &FileTree::on_buffer_undo_stack_push));

      // Restore the blocked state of all connections.
      //
      if(conn_match_count_blocked)
        conn_match_count_.block();

      if(conn_modified_changed_blocked)
        conn_modified_changed_.block();

      if(conn_undo_stack_push_blocked)
        conn_undo_stack_push_.block();
    }

    last_selected_rowref_.reset(new TreeRowRef(treestore_, Gtk::TreePath(iter)));
  }

  if(last_selected_ && last_selected_ != fileinfo &&
     last_selected_->buffer && last_selected_->buffer->is_freeable())
  {
    last_selected_->buffer.clear(); // reduce memory footprint
  }

  last_selected_ = fileinfo;

  signal_switch_buffer(fileinfo, file_index); // emit
  signal_bound_state_changed(); // emit
}

void FileTree::on_buffer_match_count_changed(int match_count)
{
  const FileTreeColumns& columns = filetree_columns();

  // There has to be a selection since we receive signal_match_count_changed()
  // from the currently selected FileBuffer.
  Gtk::TreeModel::iterator iter = get_selection()->get_selected();
  g_return_if_fail(iter);

  const int old_match_count = (*iter)[columns.matchcount];

  if(match_count == old_match_count)
    return; // spurious emission -- do nothing

  const long old_sum_matches = sum_matches_;
  propagate_match_count_change(iter, match_count - old_match_count);

  if(old_sum_matches == 0)
  {
    path_match_first_ = Gtk::TreePath(iter);
    path_match_last_  = Gtk::TreePath(iter);
  }
  else if((sum_matches_ > 0) && (old_match_count == 0 || match_count == 0))
  {
    // OK, this nightmarish-looking code below is all about adjusting the
    // [path_match_first_,path_match_last_] range.  Adjustment is necessary
    // if either a) match_count was previously 0 and iter is not already
    // in the range, or b) match_count dropped to 0 and iter is a boundary
    // of the range.
    //
    // Preconditions we definitely know at this point:
    // 1) old_sum_matches > 0
    // 2) sum_matches != old_sum_matches && sum_matches > 0
    // 3) old_match_count == 0 || match_count == 0
    // 4) old_match_count != match_count

    g_assert(old_sum_matches > 0);
    g_assert(old_match_count != match_count);

    // The range should've been set up already since old_sum_matches > 0.
    g_return_if_fail(path_match_first_ <= path_match_last_);

    Gtk::TreePath path (iter);

    if(old_match_count == 0)
    {
      g_assert(match_count > 0);
      g_return_if_fail(path != path_match_first_ && path != path_match_last_);

      // Expand the range if necessary.
      if(path < path_match_first_)
        path_match_first_ = path;
      else if(path > path_match_last_)
        path_match_last_ = path;
    }
    else
    {
      g_assert(match_count == 0);
      g_return_if_fail(path >= path_match_first_ && path <= path_match_last_);

      if(path == path_match_first_)
      {
        // Find the new start boundary of the range.
        const bool found_next = next_match_file(iter);
        g_return_if_fail(found_next);

        path_match_first_ = Gtk::TreePath(iter);
      }
      else if(path == path_match_last_)
      {
        // Find the new end boundary of the range.
        const bool found_prev = prev_match_file(iter);
        g_return_if_fail(found_prev);

        path_match_last_ = Gtk::TreePath(iter);
      }
    }

    signal_bound_state_changed(); // emit
  }
}

void FileTree::on_buffer_modified_changed()
{
  if(conn_modified_changed_.blocked()) // work around a bug in gtkmm <= 2.0.0
    return;

  const Gtk::TreeModel::iterator selected = get_selection()->get_selected();
  g_return_if_fail(selected);

  const FileInfoBasePtr base = (*selected)[filetree_columns().fileinfo];
  const FileInfoPtr fileinfo = shared_polymorphic_cast<FileInfo>(base);

  g_return_if_fail(fileinfo == last_selected_);
  g_return_if_fail(fileinfo->buffer);

  propagate_modified_change(selected, fileinfo->buffer->get_modified());
}

void FileTree::on_buffer_undo_stack_push(UndoActionPtr undo_action)
{
  g_return_if_fail(last_selected_rowref_);

  const UndoActionPtr action_shell (
      new BufferActionShell(*this, last_selected_rowref_, undo_action));

  signal_undo_stack_push(action_shell); // emit
}

int FileTree::calculate_file_index(const Gtk::TreeModel::iterator& pos)
{
  int index = 0;

  Gtk::TreeModel::iterator iter = pos->parent();

  if(iter) // calculate the parent's index first if there is one
  {
    index = calculate_file_index(iter); // recurse
    iter = iter->children().begin();
  }
  else // toplevel reached
  {
    iter = treestore_->children().begin();
  }

  const FileTreeColumns& columns = filetree_columns();

  for(; iter != pos; ++iter)
  {
    const FileInfoBasePtr base ((*iter)[columns.fileinfo]);
    g_return_val_if_fail(base, index);

    if(const DirInfoPtr dirinfo = shared_dynamic_cast<DirInfo>(base))
      index += dirinfo->file_count; // count whole directory in one step
    else
      ++index; // single file
  }

  return index;
}

void FileTree::propagate_match_count_change(const Gtk::TreeModel::iterator& pos, int difference)
{
  const FileTreeColumns& columns = filetree_columns();

  for(Gtk::TreeModel::iterator iter = pos; iter; iter = iter->parent())
  {
    const int match_count = (*iter)[columns.matchcount];
    (*iter)[columns.matchcount] = match_count + difference;
  }

  sum_matches_ += difference;

  signal_match_count_changed(); // emit
}

void FileTree::propagate_modified_change(const Gtk::TreeModel::iterator& pos, bool modified)
{
  const FileTreeColumns& columns = filetree_columns();

  treestore_->row_changed(Gtk::TreePath(pos), pos);

  for(Gtk::TreeModel::iterator iter = pos->parent(); iter; iter = iter->parent())
  {
    const FileInfoBasePtr base = (*iter)[columns.fileinfo];
    const DirInfoPtr dirinfo = shared_polymorphic_cast<DirInfo>(base);

    if(modified)
    {
      if(dirinfo->modified_count++ == 0)
        treestore_->row_changed(Gtk::TreePath(iter), iter);
    }
    else
    {
      if(--dirinfo->modified_count == 0)
        treestore_->row_changed(Gtk::TreePath(iter), iter);
    }
  }

  if(modified)
    ++toplevel_.modified_count;
  else
    --toplevel_.modified_count;

  signal_modified_count_changed(); // emit
}

void FileTree::load_file_with_fallback(const Gtk::TreeModel::iterator& iter,
                                       const FileInfoPtr& fileinfo)
{
  g_return_if_fail(!fileinfo->buffer);

  const bool old_load_failed = fileinfo->load_failed;

  try
  {
    load_file(fileinfo, fallback_encoding_);
  }
  catch(const Glib::Error& error)
  {
    fileinfo->buffer = create_error_message_buffer(error.what());
  }

  if(!fileinfo->buffer)
  {
    g_assert(fileinfo->load_failed);

    Glib::ustring message = "\302\273";
    message += Util::filename_to_utf8_fallback(Glib::path_get_basename(fileinfo->fullname));
    message += "\302\253 seems to be a binary file.";

    fileinfo->buffer = create_error_message_buffer(message);
  }

  if(old_load_failed != fileinfo->load_failed)
  {
    // Trigger signal_row_changed() because the value of fileinfo->load_failed
    // changed, which means we have to change icon and color of the row.
    treestore_->row_changed(Gtk::TreePath(iter), iter);
  }
}

Glib::RefPtr<FileBuffer> FileTree::create_error_message_buffer(const Glib::ustring& message)
{
  if(!error_pixbuf_)
    error_pixbuf_ = render_icon(Gtk::Stock::DIALOG_ERROR, Gtk::ICON_SIZE_DIALOG);

  return FileBuffer::create_with_error_message(error_pixbuf_, message);
}

} // namespace Regexxer

