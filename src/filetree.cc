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
#include "pcreshell.h"
#include "stringutils.h"

#include <gtkmm/treestore.h>
#include <gtkmm/stock.h>
#include <stack>
#include <utility>


namespace
{

typedef std::pair<std::string,Gtk::TreeModel::iterator> DirNodePair;
typedef std::list<DirNodePair>                          DirStack;


struct FileTreeColumns : public Gtk::TreeModel::ColumnRecord
{
  Gtk::TreeModelColumn<Glib::ustring>             filename;
  Gtk::TreeModelColumn<std::string>               collatekey;
  Gtk::TreeModelColumn<int>                       matchcount;
  Gtk::TreeModelColumn<Regexxer::FileInfoBasePtr> fileinfo;

  FileTreeColumns() { add(filename); add(collatekey); add(matchcount); add(fileinfo); }
};

const FileTreeColumns& filetree_columns() G_GNUC_CONST;
const FileTreeColumns& filetree_columns()
{
  static FileTreeColumns column_record;
  return column_record;
}

inline
Regexxer::FileInfoPtr get_fileinfo_from_iter(const Gtk::TreeModel::iterator& iter)
{
  const Regexxer::FileInfoBasePtr base ((*iter)[filetree_columns().fileinfo]);
  return Regexxer::FileInfoPtr::cast_dynamic(base);
}

class ScopedConnection
{
private:
  SigC::Connection connection_;

  ScopedConnection(const ScopedConnection&);
  ScopedConnection& operator=(const ScopedConnection&);

public:
  explicit ScopedConnection(const SigC::Connection& connection)
    : connection_ (connection) {}

  ~ScopedConnection() { connection_.disconnect(); }
};

class ScopedBlock
{
private:
  SigC::Connection& connection_;

  ScopedBlock(const ScopedBlock&);
  ScopedBlock& operator=(const ScopedBlock&);

public:
  explicit ScopedBlock(SigC::Connection& connection)
    : connection_ (connection) { connection_.block(); }

  ~ScopedBlock() { connection_.unblock(); }
};

class ScopedPushDir
{
private:
  DirStack& dirstack_;

  ScopedPushDir(const ScopedPushDir&);
  ScopedPushDir& operator=(const ScopedPushDir&);

public:
  ScopedPushDir(DirStack& dirstack, const std::string& dirname)
    : dirstack_ (dirstack) { dirstack_.push_back(DirNodePair(dirname, Gtk::TreeIter())); }

  ~ScopedPushDir() { dirstack_.pop_back(); }
};

} // anonymous namespace


namespace Regexxer
{

/**** Regexxer::FileTree::ErrorList ****************************************/

/* This is just a std::list<> wrapper that can be used with Util::SharedPtr<>.
 */
struct FileTree::ErrorList : public Util::SharedObject
{
  ErrorList();
  ~ErrorList();

  std::list<Glib::ustring> errors;
};

FileTree::ErrorList::ErrorList()
{}

FileTree::ErrorList::~ErrorList()
{}


/**** Regexxer::FileTree::Error ********************************************/

FileTree::Error::Error(const Util::SharedPtr<FileTree::ErrorList>& error_list)
:
  error_list_ (error_list)
{}

FileTree::Error::~Error()
{}

FileTree::Error::Error(const FileTree::Error& other)
:
  error_list_ (other.error_list_)
{}

FileTree::Error& FileTree::Error::operator=(const FileTree::Error& other)
{
  error_list_ = other.error_list_;
  return *this;
}

const std::list<Glib::ustring>& FileTree::Error::get_error_list() const
{
  return error_list_->errors;
}


/**** Regexxer::FileTree::FindData *****************************************/

struct FileTree::FindData
{
  FindData(Pcre::Pattern& pattern_, bool recursive_, bool hidden_);
  ~FindData();

  Pcre::Pattern&                        pattern;
  const bool                            recursive;
  const bool                            hidden;
  DirStack                              dirstack;
  Util::SharedPtr<FileTree::ErrorList>  error_list;

  std::list<Glib::ustring>& errors() { return error_list->errors; }

private:
  FindData(const FileTree::FindData&);
  FileTree::FindData& operator=(const FileTree::FindData&);
};

FileTree::FindData::FindData(Pcre::Pattern& pattern_, bool recursive_, bool hidden_)
:
  pattern     (pattern_),
  recursive   (recursive_),
  hidden      (hidden_),
  error_list  (new FileTree::ErrorList())
{}

FileTree::FindData::~FindData()
{}


/**** Regexxer::FileTree::FindMatchesData **********************************/

struct FileTree::FindMatchesData
{
  FindMatchesData(Pcre::Pattern& pattern_, bool multiple_);

  Pcre::Pattern&  pattern;
  const bool      multiple;
  bool            path_match_first_set;

private:
  FindMatchesData(const FileTree::FindMatchesData&);
  FileTree::FindMatchesData& operator=(const FileTree::FindMatchesData&);
};

FileTree::FindMatchesData::FindMatchesData(Pcre::Pattern& pattern_, bool multiple_)
:
  pattern              (pattern_),
  multiple             (multiple_),
  path_match_first_set (false)
{}


/**** Regexxer::FileTree ***************************************************/

FileTree::FileTree()
:
  treestore_          (Gtk::TreeStore::create(filetree_columns())),
  color_modified_     ("red"),
  sum_matches_        (0),
  fallback_encoding_  ("ISO-8859-15")
{
  set_model(treestore_);

  const FileTreeColumns& model_columns = filetree_columns();

  append_column("File", model_columns.filename);
  append_column("#",    model_columns.matchcount);

  Gtk::TreeView::Column& file_column = *get_column(0);
  Gtk::CellRenderer& file_renderer = *file_column.get_first_cell_renderer();
  file_column.set_cell_data_func(file_renderer, SigC::slot(*this, &FileTree::cell_data_func));
  file_column.set_resizable(true);

  Gtk::TreeView::Column& count_column = *get_column(1);
  Gtk::CellRenderer& count_renderer = *count_column.get_first_cell_renderer();
  count_column.set_cell_data_func(count_renderer, SigC::slot(*this, &FileTree::cell_data_func));
  count_column.set_alignment(1.0);
  count_renderer.property_xalign() = 1.0;

  treestore_->set_default_sort_func(&FileTree::default_sort_func);
  treestore_->set_sort_column_id(Gtk::TreeSortable::DEFAULT_SORT_COLUMN_ID, Gtk::SORT_ASCENDING);

  set_search_column(0);

  const Glib::RefPtr<Gtk::TreeSelection> selection = get_selection();

  selection->set_select_function(&FileTree::select_func);
  selection->signal_changed().connect(SigC::slot(*this, &FileTree::on_selection_changed));
}

FileTree::~FileTree()
{}

void FileTree::find_files(const Glib::ustring& dirname, Pcre::Pattern& pattern,
                          bool recursive, bool hidden)
{
  const std::string startdir = Glib::filename_from_utf8(dirname);
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
    const int file_count = find_recursively(startdir, find_data);
    find_increment_file_count(find_data, file_count);
  }
  catch(const Glib::FileError& error)
  {
    find_data.errors().push_back(error.what()); // collect errors but don't fail
  }

  signal_bound_state_changed(); // emit

  if(!find_data.errors().empty())
    throw Error(find_data.error_list);
}

int FileTree::get_file_count() const
{
  return toplevel_.file_count;
}

void FileTree::save_current_file()
{
  if(const Gtk::TreeModel::iterator selected = get_selection()->get_selected())
  {
    Util::SharedPtr<ErrorList> error_list;

    save_file_at_iter(selected, &error_list);

    if(error_list)
      throw Error(error_list);
  }
}

void FileTree::save_all_files()
{
  Util::SharedPtr<ErrorList> error_list;

  {
    ScopedBlock block (conn_modified_changed_);
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
    ScopedBlock block (conn_match_count_);
    FindMatchesData find_data (pattern, multiple);

    treestore_->foreach(SigC::bind(SigC::slot(*this, &FileTree::find_matches_at_iter), &find_data));
  }

  signal_bound_state_changed(); // emit
}

long FileTree::get_match_count() const
{
  return sum_matches_;
}

void FileTree::replace_all_matches(const Glib::ustring& substitution)
{
  {
    ScopedBlock block_match_count      (conn_match_count_);
    ScopedBlock block_modified_changed (conn_modified_changed_);

    treestore_->foreach(SigC::bind(
        SigC::slot(*this, &FileTree::replace_matches_at_iter),
        &substitution));
  }

  signal_bound_state_changed(); // emit
}

int FileTree::get_modified_count() const
{
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
  color_load_failed_ = get_style()->get_text(Gtk::STATE_INSENSITIVE);

  Gtk::TreeView::on_style_changed(previous_style);
}

/**** Regexxer::FileTree -- private ****************************************/

void FileTree::cell_data_func(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& iter)
{
  Gtk::CellRendererText *const renderer = static_cast<Gtk::CellRendererText*>(cell);

  const FileInfoBasePtr base = (*iter)[filetree_columns().fileinfo];

  if(const FileInfoPtr fileinfo = FileInfoPtr::cast_dynamic(base))
  {
    if(fileinfo->load_failed)
    {
      renderer->property_foreground_gdk() = color_load_failed_;
      return;
    }

    if(fileinfo->buffer && fileinfo->buffer->get_modified())
    {
      renderer->property_foreground_gdk() = color_modified_;
      return;
    }
  }
  else if(const DirInfoPtr dirinfo = DirInfoPtr::cast_dynamic(base))
  {
    if(dirinfo->modified_count > 0)
    {
      renderer->property_foreground_gdk() = color_modified_;
      return;
    }
  }

  renderer->property_foreground_gdk().reset_value();
}

// static
int FileTree::default_sort_func(const Gtk::TreeModel::iterator& lhs,
                                const Gtk::TreeModel::iterator& rhs)
{
  const FileTreeColumns& columns = filetree_columns();

  const bool lhs_isdir = DirInfoPtr::cast_dynamic(FileInfoBasePtr((*lhs)[columns.fileinfo]));
  const bool rhs_isdir = DirInfoPtr::cast_dynamic(FileInfoBasePtr((*rhs)[columns.fileinfo]));

  if(lhs_isdir != rhs_isdir)
    return (lhs_isdir) ? -1 : 1;

  const std::string lhs_key ((*lhs)[columns.collatekey]);
  const std::string rhs_key ((*rhs)[columns.collatekey]);

  return lhs_key.compare(rhs_key);
}

// static
bool FileTree::select_func(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreePath& path, bool)
{
  return get_fileinfo_from_iter(model->get_iter(path));
}

int FileTree::find_recursively(const std::string& dirname, FileTree::FindData& find_data)
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
      find_data.errors().push_back(error.what());
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

  return file_count;
}

bool FileTree::find_check_file(const std::string& basename, const std::string& fullname,
                               FileTree::FindData& find_data)
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
    const int file_count = find_recursively(fullname, find_data); // recurse
    find_increment_file_count(find_data, file_count);
  }
  else if(file_test(fullname, FILE_TEST_IS_REGULAR))
  {
    const ustring basename_utf8 (Util::filename_to_utf8_fallback(basename));

    if(find_data.pattern.match(basename_utf8) > 0)
    {
      Gtk::TreeModel::iterator iter;

      if(find_data.dirstack.empty())
      {
        iter = treestore_->prepend(); // new toplevel node
      }
      else
      {
        if(!find_data.dirstack.back().second)
          find_fill_dirstack(find_data); // build all directory nodes in the stack

        iter = treestore_->prepend(find_data.dirstack.back().second->children());
      }

      const FileTreeColumns& columns = filetree_columns();

      (*iter)[columns.filename]   = basename_utf8;
      (*iter)[columns.collatekey] = basename_utf8.collate_key();
      (*iter)[columns.fileinfo]   = FileInfoBasePtr(new FileInfo(fullname));

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

    if(pprev == pend)
      pdir->second = treestore_->prepend(); // new toplevel node
    else
      pdir->second = treestore_->prepend(pprev->second->children());

    Gtk::TreeModel::Row row (*pdir->second);

    row[columns.filename]   = dirname;
    row[columns.collatekey] = dirname.collate_key();
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
    DirInfoPtr::cast_dynamic_throw(base)->file_count += file_count;
  }

  toplevel_.file_count += file_count;
  signal_file_count_changed(); // emit
}

bool FileTree::save_file_at_iter(const Gtk::TreeModel::iterator& iter,
                                 Util::SharedPtr<ErrorList>* error_list)
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
        error_list->reset(new ErrorList());

      Glib::ustring message = "Failed to save file '";
      message += Util::filename_to_utf8_fallback(fileinfo->fullname);
      message += "': ";
      message += error.what();

      (*error_list)->errors.push_back(message);
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
    if(!fileinfo->load_failed && !fileinfo->buffer)
      load_file_with_fallback(fileinfo);

    if(fileinfo->load_failed)
      return false; // continue

    const Glib::RefPtr<FileBuffer> buffer = fileinfo->buffer;
    g_assert(buffer);

    ScopedConnection conn (buffer->signal_pulse.connect(signal_pulse.slot()));

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
                                       const Glib::ustring* substitution)
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
      const bool was_modified = buffer->get_modified();

      {
        ScopedConnection conn (buffer->signal_pulse.connect(signal_pulse.slot()));
        buffer->replace_all_matches(*substitution);
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

void FileTree::on_selection_changed()
{
  const FileTreeColumns& columns = filetree_columns();

  FileInfoPtr fileinfo;
  int file_index = 0;

  const bool conn_match_count_blocked      = conn_match_count_.blocked();
  const bool conn_modified_changed_blocked = conn_modified_changed_.blocked();

  conn_match_count_.disconnect();
  conn_modified_changed_.disconnect();

  if(const Gtk::TreeModel::iterator iter = get_selection()->get_selected())
  {
    const FileInfoBasePtr base = (*iter)[columns.fileinfo];
    fileinfo = FileInfoPtr::cast_dynamic_throw(base);

    file_index = calculate_file_index(iter) + 1;

    if(!fileinfo->buffer)
      load_file_with_fallback(fileinfo);

    if(!fileinfo->load_failed)
    {
      conn_match_count_ = fileinfo->buffer->signal_match_count_changed.
          connect(SigC::slot(*this, &FileTree::on_buffer_match_count_changed));

      conn_modified_changed_ = fileinfo->buffer->signal_modified_changed().
          connect(SigC::slot(*this, &FileTree::on_buffer_modified_changed));

      // Restore the blocked state of both connections.
      //
      if(conn_match_count_blocked)
        conn_match_count_.block();

      if(conn_modified_changed_blocked)
        conn_modified_changed_.block();
    }
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
  const Gtk::TreeModel::iterator selected = get_selection()->get_selected();
  g_return_if_fail(selected);

  const FileInfoBasePtr base = (*selected)[filetree_columns().fileinfo];
  const FileInfoPtr fileinfo = FileInfoPtr::cast_dynamic_throw(base);

  g_return_if_fail(fileinfo == last_selected_);
  g_return_if_fail(fileinfo->buffer);

  propagate_modified_change(selected, fileinfo->buffer->get_modified());
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

    if(const DirInfoPtr dirinfo = DirInfoPtr::cast_dynamic(base))
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
    const DirInfoPtr dirinfo = DirInfoPtr::cast_dynamic_throw(base);

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

void FileTree::load_file_with_fallback(const FileInfoPtr& fileinfo)
{
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
    g_return_if_fail(fileinfo->load_failed);

    Glib::ustring message = "\302\273";
    message += Util::filename_to_utf8_fallback(Glib::path_get_basename(fileinfo->fullname));
    message += "\302\253 seems to be a binary file.";

    fileinfo->buffer = create_error_message_buffer(message);
  }
}

Glib::RefPtr<FileBuffer> FileTree::create_error_message_buffer(const Glib::ustring& message)
{
  if(!error_pixbuf_)
    error_pixbuf_ = render_icon(Gtk::Stock::DIALOG_ERROR, Gtk::ICON_SIZE_DIALOG);

  return FileBuffer::create_with_error_message(error_pixbuf_, message);
}

} // namespace Regexxer

