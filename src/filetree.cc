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

#include "filetree.h"
#include "filetreeprivate.h"
#include "globalstrings.h"
#include "pcreshell.h"
#include "stringutils.h"
#include "translation.h"

#include <gmodule.h>
#include <glibmm.h>
#include <gconfmm/client.h>
#include <gtkmm/stock.h>

#include <config.h>

/*
 * Custom widget creation function for libglade.
 */
extern "C" G_MODULE_EXPORT
GtkWidget* regexxer_create_file_tree(char*, char*, char*, int, int)
{
  try
  {
    Gtk::Widget *const widget = new Regexxer::FileTree();
    widget->show();
    return Gtk::manage(widget)->gobj();
  }
  catch (...)
  {
    g_return_val_if_reached(0);
  }
}

namespace Regexxer
{

using namespace Regexxer::FileTreePrivate;

/**** Regexxer::FileTree ***************************************************/

FileTree::FileTree()
:
  treestore_      (Gtk::TreeStore::create(FileTreeColumns::instance())),
  color_modified_ ("#DF421E"), // accent red
  sum_matches_    (0)
{
  using namespace Gtk;
  using sigc::mem_fun;

  set_model(treestore_);
  const FileTreeColumns& model_columns = FileTreeColumns::instance();

  treestore_->set_default_sort_func(&default_sort_func);
  treestore_->set_sort_func(model_columns.collatekey, &collatekey_sort_func);
  treestore_->set_sort_column(TreeStore::DEFAULT_SORT_COLUMN_ID, SORT_ASCENDING);

  treestore_->signal_rows_reordered()
      .connect(mem_fun(*this, &FileTree::on_treestore_rows_reordered));

  {
    Column *const column = new Column(_("File"));
    append_column(*manage(column));

    CellRendererPixbuf *const cell_icon = new CellRendererPixbuf();
    column->pack_start(*manage(cell_icon), false);

    CellRendererText *const cell_filename = new CellRendererText();
    column->pack_start(*manage(cell_filename));

    column->add_attribute(cell_filename->property_text(), model_columns.filename);
    column->set_cell_data_func(*cell_icon,     mem_fun(*this, &FileTree::icon_cell_data_func));
    column->set_cell_data_func(*cell_filename, mem_fun(*this, &FileTree::text_cell_data_func));

    column->set_resizable(true);
    column->set_expand(true);

    column->set_sort_column(model_columns.collatekey);
  }
  {
    Column *const column = new Column(_("#"));
    append_column(*manage(column));

    CellRendererText *const cell_matchcount = new CellRendererText();
    column->pack_start(*manage(cell_matchcount));

    column->add_attribute(cell_matchcount->property_text(), model_columns.matchcount);
    column->set_cell_data_func(*cell_matchcount, mem_fun(*this, &FileTree::text_cell_data_func));

    column->set_alignment(1.0);
    cell_matchcount->property_xalign() = 1.0;

    column->set_sort_column(model_columns.matchcount);
  }

  set_search_column(model_columns.filename);

  const Glib::RefPtr<TreeSelection> selection = get_selection();

  selection->set_select_function(&FileTree::select_func);
  selection->signal_changed().connect(mem_fun(*this, &FileTree::on_selection_changed));

  Gnome::Conf::Client::get_default_client()
      ->signal_value_changed().connect(mem_fun(*this, &FileTree::on_conf_value_changed));
}

FileTree::~FileTree()
{}

void FileTree::find_files(const std::string& dirname, Pcre::Pattern& pattern,
                          bool recursive, bool hidden)
{
  FindData find_data (pattern, recursive, hidden);

  const bool modified_count_changed = (toplevel_.modified_count != 0);

  treestore_->clear();

  toplevel_.file_count     = 0;
  toplevel_.modified_count = 0;
  sum_matches_ = 0;

  signal_bound_state_changed(); // emit
  signal_match_count_changed(); // emit

  if (modified_count_changed)
    signal_modified_count_changed(); // emit

  find_recursively(dirname, find_data);

  // Work around a strange misbehavior: the tree is kept sorted while the
  // file search is in progress, which causes the scroll offset to change
  // slightly.  This in turn confuses TreeView::set_cursor() -- the first
  // call after the tree was completely filled just doesn't scroll.
  if (toplevel_.file_count > 0)
    scroll_to_row(Gtk::TreeModel::Path(1u, 0));

  signal_bound_state_changed(); // emit

  if (!find_data.error_list->empty())
    throw Error(find_data.error_list);
}

int FileTree::get_file_count() const
{
  g_return_val_if_fail(toplevel_.file_count >= 0, 0);
  return toplevel_.file_count;
}

void FileTree::save_current_file()
{
  if (const Gtk::TreeModel::iterator selected = get_selection()->get_selected())
  {
    Util::SharedPtr<MessageList> error_list (new MessageList());

    {
      Util::ScopedBlock block (conn_modified_changed_);
      save_file_at_iter(selected, error_list);
    }

    if (!error_list->empty())
      throw Error(error_list);
  }
}

void FileTree::save_all_files()
{
  Util::SharedPtr<MessageList> error_list (new MessageList());

  {
    Util::ScopedBlock block (conn_modified_changed_);

    treestore_->foreach_iter(sigc::bind(
        sigc::mem_fun(*this, &FileTree::save_file_at_iter),
        sigc::ref(error_list)));
  }

  if (!error_list->empty())
    throw Error(error_list);
}

void FileTree::select_first_file()
{
  if (sum_matches_ > 0)
    expand_and_select(path_match_first_);
}

bool FileTree::select_next_file(bool move_forward)
{
  if (Gtk::TreeModel::iterator iter = get_selection()->get_selected())
  {
    Gtk::TreeModel::Path collapse;

    if ((move_forward) ? next_match_file(iter, &collapse) : prev_match_file(iter, &collapse))
    {
      if (!collapse.empty())
        collapse_row(collapse);

      expand_and_select(Gtk::TreeModel::Path(iter));
      return true;
    }
  }

  return false;
}

BoundState FileTree::get_bound_state()
{
  BoundState bound = BOUND_FIRST | BOUND_LAST;

  if (sum_matches_ > 0)
  {
    if (const Gtk::TreeModel::iterator iter = get_selection()->get_selected())
    {
      Gtk::TreeModel::Path path (iter);

      if (path > path_match_first_)
        bound &= ~BOUND_FIRST;

      if (path < path_match_last_)
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
    FindMatchesData    find_data  (pattern, multiple);

    treestore_->foreach(sigc::bind(
        sigc::mem_fun(*this, &FileTree::find_matches_at_path_iter),
        sigc::ref(find_data)));
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
    ScopedBlockSorting block_sort   (*this);
    ReplaceMatchesData replace_data (*this, substitution);

    treestore_->foreach(sigc::bind(
        sigc::mem_fun(*this, &FileTree::replace_matches_at_path_iter),
        sigc::ref(replace_data)));

    // Adjust the boundary range if the operation has been interrupted.
    if (sum_matches_ > 0)
    {
      Gtk::TreeModel::iterator first = treestore_->get_iter(path_match_first_);

      if ((*first)[FileTreeColumns::instance().matchcount] == 0 && next_match_file(first))
        path_match_first_ = first;
    }

    signal_undo_stack_push(replace_data.undo_stack); // emit
  }

  signal_bound_state_changed(); // emit
}

int FileTree::get_modified_count() const
{
  g_return_val_if_fail(toplevel_.modified_count >= 0, 0);
  return toplevel_.modified_count;
}

/**** Regexxer::FileTree -- protected **************************************/

void FileTree::on_style_changed(const Glib::RefPtr<Gtk::Style>& previous_style)
{
  const Glib::ustring detail = "regexxer-filetree";

  pixbuf_directory_   = render_icon(Gtk::Stock::DIRECTORY,     Gtk::ICON_SIZE_MENU, detail);
  pixbuf_file_        = render_icon(Gtk::Stock::FILE,          Gtk::ICON_SIZE_MENU, detail);
  pixbuf_load_failed_ = render_icon(Gtk::Stock::MISSING_IMAGE, Gtk::ICON_SIZE_MENU, detail);

  color_load_failed_ = get_style()->get_text(Gtk::STATE_INSENSITIVE);

  Gtk::TreeView::on_style_changed(previous_style);
}

/**** Regexxer::FileTree -- private ****************************************/

void FileTree::icon_cell_data_func(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& iter)
{
  Gtk::CellRendererPixbuf& renderer = dynamic_cast<Gtk::CellRendererPixbuf&>(*cell);
  const FileInfoBasePtr    infobase = (*iter)[FileTreeColumns::instance().fileinfo];

  if (const FileInfoPtr fileinfo = shared_dynamic_cast<FileInfo>(infobase))
  {
    renderer.property_pixbuf() = (fileinfo->load_failed) ? pixbuf_load_failed_ : pixbuf_file_;
  }
  else if (shared_dynamic_cast<DirInfo>(infobase))
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
  const FileInfoBasePtr  infobase = (*iter)[FileTreeColumns::instance().fileinfo];

  const Gdk::Color* color = 0;

  if (const FileInfoPtr fileinfo = shared_dynamic_cast<FileInfo>(infobase))
  {
    if (fileinfo->load_failed)
      color = &color_load_failed_;
    else if (fileinfo->buffer && fileinfo->buffer->get_modified())
      color = &color_modified_;
  }
  else if (const DirInfoPtr dirinfo = shared_dynamic_cast<DirInfo>(infobase))
  {
    if (dirinfo->modified_count > 0)
      color = &color_modified_;
  }

  if (color)
    renderer.property_foreground_gdk() = *color;
  else
    renderer.property_foreground_gdk().reset_value();

  if (color == &color_modified_)
    renderer.property_style() = Pango::STYLE_OBLIQUE;
  else
    renderer.property_style().reset_value();
}

// static
bool FileTree::select_func(const Glib::RefPtr<Gtk::TreeModel>& model,
                           const Gtk::TreeModel::Path& path, bool)
{
  // Don't allow selection of directory nodes.
  return (get_fileinfo_from_iter(model->get_iter(path)) != 0);
}

void FileTree::find_recursively(const std::string& dirname, FindData& find_data)
{
  using namespace Glib;

  try
  {
    int file_count = 0;
    Dir dir (dirname);

    for (Dir::iterator pos = dir.begin(); pos != dir.end(); ++pos)
    {
      if (signal_pulse()) // emit
        break;

      const std::string filename = *pos;

      if (!find_data.hidden && *filename.begin() == '.')
        continue;

      const std::string fullname = build_filename(dirname, filename);

      if (file_test(fullname, FILE_TEST_IS_SYMLINK))
        continue; // ignore symbolic links

      if (find_data.recursive && file_test(fullname, FILE_TEST_IS_DIR))
      {
        // Put the directory name on the stack instead of creating a new node
        // immediately.  The corresponding node will be created on demand if
        // there's actually a matching file in the directory or one of its
        // subdirectories.
        ScopedPushDir pushdir (find_data.dirstack, fullname);
        find_recursively(fullname, find_data); // recurse
      }
      else if (file_test(fullname, FILE_TEST_IS_REGULAR))
      {
        const ustring basename = Glib::filename_display_basename(fullname);

        if (find_data.pattern.match(basename) > 0)
        {
          find_add_file(basename, fullname, find_data);
          ++file_count;
        }
      }
    }

    find_increment_file_count(find_data, file_count);
  }
  catch (const FileError& error)
  {
    // Collect errors but don't interrupt the search.
    find_data.error_list->push_back(error.what());
  }
}

void FileTree::find_add_file(const Glib::ustring& basename, const std::string& fullname,
                             FindData& find_data)
{
  // Build the collate key with a leading '1' so that directories always
  // come first (they have a leading '0').  This is simpler and faster
  // than explicitely checking for directories in the sort function.
  std::string collate_key (1, '1');
  collate_key += basename.collate_key();

  const FileInfoBasePtr fileinfo (new FileInfo(fullname));

  Gtk::TreeModel::Row row;

  if (find_data.dirstack.empty())
  {
    row = *treestore_->prepend(); // new toplevel node
  }
  else
  {
    if (!find_data.dirstack.back().second)
      find_fill_dirstack(find_data); // build all directory nodes in the stack

    row = *treestore_->prepend(find_data.dirstack.back().second->children());
  }

  const FileTreeColumns& columns = FileTreeColumns::instance();

  row[columns.filename]   = basename;
  row[columns.collatekey] = collate_key;
  row[columns.fileinfo]   = fileinfo;
}

void FileTree::find_fill_dirstack(FindData& find_data)
{
  const FileTreeColumns& columns = FileTreeColumns::instance();

  const DirStack::iterator pend  = find_data.dirstack.end();
  DirStack::iterator       pprev = pend;

  for (DirStack::iterator pdir = find_data.dirstack.begin(); pdir != pend; pprev = pdir++)
  {
    if (pdir->second) // node already created
      continue;

    const Glib::ustring dirname = Glib::filename_display_basename(pdir->first);

    // Build the collate key with a leading '0' so that directories always
    // come first.  This is simpler and faster than explicitely checking for
    // directories in the sort function.
    std::string collate_key (1, '0');
    collate_key += dirname.collate_key();

    const FileInfoBasePtr dirinfo (new DirInfo());

    if (pprev == pend)
      pdir->second = treestore_->prepend(); // new toplevel node
    else
      pdir->second = treestore_->prepend(pprev->second->children());

    Gtk::TreeModel::Row row = *pdir->second;

    row[columns.filename]   = dirname;
    row[columns.collatekey] = collate_key;
    row[columns.fileinfo]   = dirinfo;
  }
}

void FileTree::find_increment_file_count(FindData& find_data, int file_count)
{
  if (file_count <= 0)
    return;

  const FileTreeColumns& columns = FileTreeColumns::instance();

  const DirStack::iterator pbegin = find_data.dirstack.begin();
  DirStack::iterator       pdir   = find_data.dirstack.end();

  while (pdir != pbegin)
  {
    const FileInfoBasePtr base = (*(--pdir)->second)[columns.fileinfo];
    shared_polymorphic_cast<DirInfo>(base)->file_count += file_count;
  }

  toplevel_.file_count += file_count;
  signal_file_count_changed(); // emit
}

bool FileTree::save_file_at_iter(const Gtk::TreeModel::iterator& iter,
                                 const Util::SharedPtr<MessageList>& error_list)
{
  const FileInfoPtr fileinfo = get_fileinfo_from_iter(iter);

  if (fileinfo && fileinfo->buffer && fileinfo->buffer->get_modified())
  {
    try
    {
      save_file(fileinfo);
    }
    catch (const Glib::Error& error)
    {
      error_list->push_back(Util::compose(_("Failed to save file \342\200\234%1\342\200\235: %2"),
                                          Glib::filename_display_basename(fileinfo->fullname),
                                          error.what()));
    }

    if (!fileinfo->buffer->get_modified())
      propagate_modified_change(iter, false);

    if (fileinfo != last_selected_ && fileinfo->buffer->is_freeable())
      Glib::RefPtr<FileBuffer>().swap(fileinfo->buffer); // reduce memory footprint
  }

  return false;
}

bool FileTree::find_matches_at_path_iter(const Gtk::TreeModel::Path& path,
                                         const Gtk::TreeModel::iterator& iter,
                                         FindMatchesData& find_data)
{
  if (signal_pulse()) // emit
    return true;

  if (const FileInfoPtr fileinfo = get_fileinfo_from_iter(iter))
  {
    if (!fileinfo->buffer)
      load_file_with_fallback(iter, fileinfo);

    if (fileinfo->load_failed)
      return false; // continue

    const Glib::RefPtr<FileBuffer> buffer = fileinfo->buffer;
    g_assert(buffer);

    Util::ScopedConnection conn (buffer->signal_pulse.connect(signal_pulse.make_slot()));

    const int old_match_count = buffer->get_match_count();

    // Optimize the common case and construct the feedback slot only if there
    // are actually any handlers connected to the signal.  find_matches() can
    // then check whether the slot is empty to avoid providing arguments that
    // are never going to be used.
    const int new_match_count =
        buffer->find_matches(find_data.pattern, find_data.multiple, (signal_feedback.empty())
                             ? sigc::slot<void, int, const Glib::ustring&>()
                             : sigc::bind(signal_feedback.make_slot(), fileinfo));

    if (new_match_count > 0)
    {
      if (!find_data.path_match_first_set)
      {
        find_data.path_match_first_set = true;
        path_match_first_ = path;
      }

      path_match_last_ = path;
    }

    if (new_match_count != old_match_count)
      propagate_match_count_change(iter, new_match_count - old_match_count);

    if (fileinfo != last_selected_ && buffer->is_freeable())
      Glib::RefPtr<FileBuffer>().swap(fileinfo->buffer); // reduce memory footprint
  }

  return false;
}

bool FileTree::replace_matches_at_path_iter(const Gtk::TreeModel::Path& path,
                                            const Gtk::TreeModel::iterator& iter,
                                            ReplaceMatchesData& replace_data)
{
  if (signal_pulse()) // emit
    return true;

  const FileInfoPtr fileinfo = get_fileinfo_from_iter(iter);

  if (fileinfo && fileinfo->buffer)
  {
    const Glib::RefPtr<FileBuffer> buffer = fileinfo->buffer;

    const int match_count = buffer->get_match_count();

    if (match_count > 0)
    {
      path_match_first_ = path;

      if (fileinfo != last_selected_)
        replace_data.row_reference.reset(new TreeRowRef(treestore_, path));
      else
        replace_data.row_reference = last_selected_rowref_;

      const bool was_modified = buffer->get_modified();

      {
        // Redirect the buffer's signal_undo_stack_push() in order to create
        // a single user action object for all replacements in all buffers.
        // Note that the caller must block conn_undo_stack_push_ to avoid
        // double notification.
        Util::ScopedConnection conn1 (buffer->signal_undo_stack_push
                                        .connect(replace_data.slot_undo_stack_push));
        Util::ScopedConnection conn2 (buffer->signal_pulse.connect(signal_pulse.make_slot()));

        buffer->replace_all_matches(replace_data.substitution);
      }

      const bool is_modified = buffer->get_modified();

      if (was_modified != is_modified)
        propagate_modified_change(iter, is_modified);

      propagate_match_count_change(iter, buffer->get_match_count() - match_count);
    }
  }

  return false;
}

void FileTree::expand_and_select(const Gtk::TreeModel::Path& path)
{
  expand_to_path(path);
  set_cursor(path);
}

void FileTree::on_treestore_rows_reordered(const Gtk::TreeModel::Path& path,
                                           const Gtk::TreeModel::iterator& iter, int*)
{
  if (sum_matches_ > 0)
  {
    const FileTreeColumns& columns = FileTreeColumns::instance();
    bool bounds_changed = false;

    if (path.is_ancestor(path_match_first_))
    {
      Gtk::TreeModel::iterator first = iter->children().begin();

      while (first->children() && (*first)[columns.matchcount] > 0)
        first = first->children().begin();

      if ((*first)[columns.matchcount] == 0)
      {
        const bool found_first = next_match_file(first);
        g_return_if_fail(found_first);
      }

      path_match_first_ = first;
      bounds_changed = true;
    }

    if (path.is_ancestor(path_match_last_))
    {
      Gtk::TreeModel::iterator last = iter->children()[iter->children().size() - 1];

      while (last->children() && (*last)[columns.matchcount] > 0)
        last = last->children()[last->children().size() - 1];

      if ((*last)[columns.matchcount] == 0)
      {
        const bool found_last = prev_match_file(last);
        g_return_if_fail(found_last);
      }

      path_match_last_ = last;
      bounds_changed = true;
    }

    if (bounds_changed)
      signal_bound_state_changed(); // emit
  }
}

void FileTree::on_selection_changed()
{
  last_selected_rowref_.reset();

  conn_match_count_     .disconnect();
  conn_modified_changed_.disconnect();
  conn_undo_stack_push_ .disconnect();

  FileInfoPtr fileinfo;
  int file_index = 0;

  if (const Gtk::TreeModel::iterator iter = get_selection()->get_selected())
  {
    const FileInfoBasePtr base = (*iter)[FileTreeColumns::instance().fileinfo];

    fileinfo   = shared_polymorphic_cast<FileInfo>(base);
    file_index = calculate_file_index(iter) + 1;

    if (!fileinfo->buffer)
      load_file_with_fallback(iter, fileinfo);

    if (!fileinfo->load_failed)
    {
      conn_match_count_ = fileinfo->buffer->signal_match_count_changed.
          connect(sigc::mem_fun(*this, &FileTree::on_buffer_match_count_changed));

      conn_modified_changed_ = fileinfo->buffer->signal_modified_changed().
          connect(sigc::mem_fun(*this, &FileTree::on_buffer_modified_changed));

      conn_undo_stack_push_ = fileinfo->buffer->signal_undo_stack_push.
          connect(sigc::mem_fun(*this, &FileTree::on_buffer_undo_stack_push));
    }

    last_selected_rowref_.reset(new TreeRowRef(treestore_, Gtk::TreeModel::Path(iter)));
  }

  if (last_selected_ && last_selected_ != fileinfo &&
      last_selected_->buffer && last_selected_->buffer->is_freeable())
  {
    Glib::RefPtr<FileBuffer>().swap(last_selected_->buffer); // reduce memory footprint
  }

  last_selected_ = fileinfo;

  signal_switch_buffer(fileinfo, file_index); // emit
  signal_bound_state_changed(); // emit
}

void FileTree::on_buffer_match_count_changed()
{
  const FileTreeColumns& columns = FileTreeColumns::instance();

  // There has to be a selection since we receive signal_match_count_changed()
  // from the currently selected FileBuffer.
  Gtk::TreeModel::iterator iter = get_selection()->get_selected();
  g_return_if_fail(iter);

  const FileInfoBasePtr base = (*iter)[columns.fileinfo];
  const FileInfoPtr fileinfo = shared_polymorphic_cast<FileInfo>(base);

  g_return_if_fail(fileinfo->buffer);

  const int match_count     = fileinfo->buffer->get_match_count();
  const int old_match_count = (*iter)[columns.matchcount];

  if (match_count == old_match_count)
    return; // spurious emission -- do nothing

  const long old_sum_matches = sum_matches_;
  propagate_match_count_change(iter, match_count - old_match_count);

  if (old_sum_matches == 0)
  {
    path_match_first_ = iter;
    path_match_last_  = path_match_first_;
  }
  else if ((sum_matches_ > 0) && (old_match_count == 0 || match_count == 0))
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

    Gtk::TreeModel::Path path (iter);

    if (old_match_count == 0)
    {
      g_assert(match_count > 0);
      g_return_if_fail(path != path_match_first_ && path != path_match_last_);

      // Expand the range if necessary.
      if (path < path_match_first_)
        path_match_first_ = path;
      else if (path > path_match_last_)
        path_match_last_ = path;
    }
    else
    {
      g_assert(match_count == 0);
      g_return_if_fail(path >= path_match_first_ && path <= path_match_last_);

      if (path == path_match_first_)
      {
        // Find the new start boundary of the range.
        const bool found_next = next_match_file(iter);
        g_return_if_fail(found_next);

        path_match_first_ = iter;
      }
      else if (path == path_match_last_)
      {
        // Find the new end boundary of the range.
        const bool found_prev = prev_match_file(iter);
        g_return_if_fail(found_prev);

        path_match_last_ = iter;
      }
    }

    signal_bound_state_changed(); // emit
  }
}

void FileTree::on_buffer_modified_changed()
{
  const Gtk::TreeModel::iterator selected = get_selection()->get_selected();
  g_return_if_fail(selected);

  const FileInfoBasePtr base = (*selected)[FileTreeColumns::instance().fileinfo];
  const FileInfoPtr fileinfo = shared_polymorphic_cast<FileInfo>(base);

  g_return_if_fail(fileinfo == last_selected_);
  g_return_if_fail(fileinfo->buffer);

  propagate_modified_change(selected, fileinfo->buffer->get_modified());
}

void FileTree::on_buffer_undo_stack_push(UndoActionPtr undo_action)
{
  g_return_if_fail(last_selected_rowref_);

  const UndoActionPtr action_shell (new BufferActionShell(*this, last_selected_rowref_, undo_action));

  signal_undo_stack_push(action_shell); // emit
}

int FileTree::calculate_file_index(const Gtk::TreeModel::iterator& pos)
{
  int index = 0;

  Gtk::TreeModel::iterator iter = pos->parent();

  if (iter) // calculate the parent's index first if there is one
    index = calculate_file_index(iter); // recurse

  const FileTreeColumns& columns = FileTreeColumns::instance();

  for (iter = iter->children().begin(); iter != pos; ++iter)
  {
    const FileInfoBasePtr base = (*iter)[columns.fileinfo];
    g_return_val_if_fail(base, index);

    if (const DirInfoPtr dirinfo = shared_dynamic_cast<DirInfo>(base))
      index += dirinfo->file_count; // count whole directory in one step
    else
      ++index; // single file
  }

  return index;
}

void FileTree::propagate_match_count_change(const Gtk::TreeModel::iterator& pos, int difference)
{
  const FileTreeColumns& columns = FileTreeColumns::instance();

  for (Gtk::TreeModel::iterator iter = pos; iter; iter = iter->parent())
  {
    const int match_count = (*iter)[columns.matchcount];
    (*iter)[columns.matchcount] = match_count + difference;
  }

  sum_matches_ += difference;

  signal_match_count_changed(); // emit
}

void FileTree::propagate_modified_change(const Gtk::TreeModel::iterator& pos, bool modified)
{
  const int difference = (modified) ? 1 : -1;
  const FileTreeColumns& columns = FileTreeColumns::instance();

  Gtk::TreeModel::Path path (pos);
  treestore_->row_changed(path, pos);

  for (Gtk::TreeModel::iterator iter = pos->parent(); iter && path.up(); iter = iter->parent())
  {
    const FileInfoBasePtr base = (*iter)[columns.fileinfo];
    const DirInfoPtr dirinfo = shared_polymorphic_cast<DirInfo>(base);

    dirinfo->modified_count += difference;

    // Update the view only if the count flipped from 0 to 1 or vice versa.
    if (dirinfo->modified_count == int(modified))
      treestore_->row_changed(path, iter);
  }

  toplevel_.modified_count += difference;

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
  catch (const Glib::Error& error)
  {
    fileinfo->buffer = FileBuffer::create_with_error_message(
        render_icon(Gtk::Stock::DIALOG_ERROR, Gtk::ICON_SIZE_DIALOG), error.what());
  }
  catch (const ErrorBinaryFile&)
  {
    const Glib::ustring filename = (*iter)[FileTreeColumns::instance().filename];

    fileinfo->buffer = FileBuffer::create_with_error_message(
        render_icon(Gtk::Stock::DIALOG_ERROR, Gtk::ICON_SIZE_DIALOG),
        Util::compose(_("\342\200\234%1\342\200\235 seems to be a binary file."), filename));
  }

  if (old_load_failed != fileinfo->load_failed)
  {
    // Trigger signal_row_changed() because the value of fileinfo->load_failed
    // changed, which means we have to change icon and color of the row.
    treestore_->row_changed(Gtk::TreeModel::Path(iter), iter);
  }
}

void FileTree::on_conf_value_changed(const Glib::ustring& key, const Gnome::Conf::Value& value)
{
  if (value.get_type() == Gnome::Conf::VALUE_STRING)
  {
    if (key.raw() == conf_key_fallback_encoding)
      fallback_encoding_ = value.get_string();
  }
}

} // namespace Regexxer
