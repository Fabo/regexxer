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

#include "filelist.h"
#include "pcreshell.h"
#include "stringutils.h"

#include <gtkmm/liststore.h>
#include <gtkmm/stock.h>


namespace
{

struct FileListColumns : public Gtk::TreeModel::ColumnRecord
{
  Gtk::TreeModelColumn<Glib::ustring>         filename;
  Gtk::TreeModelColumn<std::string>           collatekey;
  Gtk::TreeModelColumn<int>                   matchcount;
  Gtk::TreeModelColumn<Regexxer::FileInfoPtr> fileinfo;

  FileListColumns() { add(filename); add(collatekey); add(matchcount); add(fileinfo); }
};

const FileListColumns& filelist_columns() G_GNUC_CONST;
const FileListColumns& filelist_columns()
{
  static FileListColumns column_record;
  return column_record;
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

} // anonymous namespace


namespace Regexxer
{

/**** Regexxer::FileList::ErrorList ****************************************/

/* This is just a std::list<> wrapper that can be used with Util::SharedPtr<>.
 */
struct FileList::ErrorList : public Util::SharedObject
{
  ErrorList();
  ~ErrorList();

  std::list<Glib::ustring> errors;
};

FileList::ErrorList::ErrorList()
{}

FileList::ErrorList::~ErrorList()
{}


/**** Regexxer::FileList::Error ********************************************/

FileList::Error::Error(const Util::SharedPtr<FileList::ErrorList>& error_list)
:
  error_list_ (error_list)
{}

FileList::Error::~Error()
{}

FileList::Error::Error(const FileList::Error& other)
:
  error_list_ (other.error_list_)
{}

FileList::Error& FileList::Error::operator=(const FileList::Error& other)
{
  error_list_ = other.error_list_;
  return *this;
}

const std::list<Glib::ustring>& FileList::Error::get_error_list() const
{
  return error_list_->errors;
}


/**** Regexxer::FileList::FindData *****************************************/

struct FileList::FindData
{
  FindData(const Glib::ustring&   pattern_,
           std::string::size_type chop_off_,
           bool recursive_, bool hidden_);
  ~FindData();

  Pcre::Pattern                         pattern;
  const std::string::size_type          chop_off;
  const bool                            recursive;
  const bool                            hidden;
  Util::SharedPtr<FileList::ErrorList>  error_list;

  std::list<Glib::ustring>& errors() { return error_list->errors; }
};

FileList::FindData::FindData(const Glib::ustring&   pattern_,
                             std::string::size_type chop_off_,
                             bool recursive_, bool hidden_)
:
  pattern     (pattern_),
  chop_off    (chop_off_),
  recursive   (recursive_),
  hidden      (hidden_),
  error_list  (new FileList::ErrorList())
{}

FileList::FindData::~FindData()
{}


/**** Regexxer::FileList ***************************************************/

FileList::FileList()
:
  liststore_          (Gtk::ListStore::create(filelist_columns())),
  color_modified_     ("red"),
  file_count_         (0),
  modified_count_     (0),
  sum_matches_        (0),
  fallback_encoding_  ("ISO-8859-15")
{
  set_model(liststore_);

  const FileListColumns& model_columns = filelist_columns();

  append_column("File", model_columns.filename);
  append_column("#",    model_columns.matchcount);

  Gtk::TreeView::Column& file_column = *get_column(0);
  Gtk::CellRenderer& file_renderer = *file_column.get_first_cell_renderer();
  file_column.set_cell_data_func(file_renderer, SigC::slot(*this, &FileList::cell_data_func));
  file_column.set_resizable(true);

  Gtk::TreeView::Column& count_column = *get_column(1);
  Gtk::CellRenderer& count_renderer = *count_column.get_first_cell_renderer();
  count_column.set_cell_data_func(count_renderer, SigC::slot(*this, &FileList::cell_data_func));
  count_column.set_alignment(1.0);
  count_renderer.property_xalign() = 1.0;

  liststore_->set_sort_func(model_columns.collatekey.index(), &FileList::collatekey_sort_func);
  liststore_->set_sort_column_id(model_columns.collatekey, Gtk::SORT_ASCENDING);

  set_search_column(0);

  get_selection()->signal_changed().connect(SigC::slot(*this, &FileList::on_selection_changed));
}

FileList::~FileList()
{}

void FileList::find_files(const Glib::ustring& dirname,
                          const Glib::ustring& pattern,
                          bool recursive, bool hidden)
{
  const std::string startdir = Glib::filename_from_utf8(dirname);
  std::string::size_type chop_off = startdir.length();

  if(chop_off > 0 && *startdir.rbegin() != G_DIR_SEPARATOR)
    ++chop_off;

  // The FindData ctor may throw Pcre::Error.
  FindData find_data (Util::shell_pattern_to_regex(pattern), chop_off, recursive, hidden);

  const bool modified_count_changed = (modified_count_ != 0);

  get_selection()->unselect_all(); // workaround for GTK+ <= 2.0.6 (#94868)
  liststore_->clear();

  // Don't keep the pixbuf around if we don't need it.
  // It's recreated on demand if necessary.
  error_pixbuf_.clear();

  file_count_     = 0;
  modified_count_ = 0;
  sum_matches_    = 0;

  signal_bound_state_changed(); // emit
  signal_match_count_changed(); // emit

  if(modified_count_changed)
    signal_modified_count_changed(); // emit

  try
  {
    find_recursively(startdir, find_data);
  }
  catch(const Glib::FileError& error)
  {
    find_data.errors().push_back(error.what()); // collect errors but don't fail
  }

  signal_bound_state_changed(); // emit

  if(!find_data.errors().empty())
    throw Error(find_data.error_list);
}

int FileList::get_file_count() const
{
  return file_count_;
}

void FileList::save_current_file()
{
  if(const Gtk::TreeModel::iterator iter = get_selection()->get_selected())
  {
    const FileInfoPtr fileinfo = (*iter)[filelist_columns().fileinfo];

    try
    {
      if(fileinfo->buffer)
        save_file(fileinfo);
    }
    catch(const Glib::Error& error)
    {
      const Util::SharedPtr<ErrorList> error_list (new ErrorList());

      Glib::ustring message = "Failed to save file '";
      message += Util::filename_to_utf8_fallback(fileinfo->fullname);
      message += "': ";
      message += error.what();

      error_list->errors.push_back(message);

      throw Error(error_list);
    }
  }
}

void FileList::save_all_files()
{
  const Util::SharedPtr<ErrorList> error_list (new ErrorList());

  const FileListColumns& columns = filelist_columns();
  int new_modified_count = modified_count_;

  for(Gtk::TreeModel::iterator iter = liststore_->children().begin(); iter; ++iter)
  {
    const FileInfoPtr fileinfo = (*iter)[columns.fileinfo];

    if(fileinfo->buffer && fileinfo->buffer->get_modified())
    {
      try
      {
        save_file(fileinfo);
        --new_modified_count;
      }
      catch(const Glib::Error& error)
      {
        Glib::ustring message = "Failed to save file '";
        message += Util::filename_to_utf8_fallback(fileinfo->fullname);
        message += "': ";
        message += error.what();

        error_list->errors.push_back(message);
      }

      liststore_->row_changed(Gtk::TreePath(iter), iter);
    }
  }

  if(modified_count_ != new_modified_count)
  {
    modified_count_ = new_modified_count;
    signal_modified_count_changed(); // emit
  }

  if(!error_list->errors.empty())
    throw Error(error_list);
}

void FileList::select_first_file()
{
  const FileListColumns& columns = filelist_columns();

  for(Gtk::TreeModel::iterator iter = liststore_->children().begin(); iter; ++iter)
  {
    if((*iter)[columns.matchcount] > 0)
    {
      get_selection()->select(iter);
      scroll_to_row(Gtk::TreePath(iter), 0.5);
      return;
    }
  }
}

bool FileList::select_next_file(bool move_forward)
{
  const FileListColumns& columns = filelist_columns();
  const Glib::RefPtr<Gtk::TreeSelection> selection = get_selection();

  if(Gtk::TreeModel::iterator iter = selection->get_selected())
  {
    if(move_forward)
    {
      while(++iter)
        if((*iter)[columns.matchcount] > 0)
        {
          selection->select(iter);
          scroll_to_row(Gtk::TreePath(iter), 0.5);
          return true;
        }
    }
    else
    {
      Gtk::TreePath path (iter);

      while(path.prev())
        if((*liststore_->get_iter(path))[columns.matchcount] > 0)
        {
          selection->select(path);
          scroll_to_row(path, 0.5);
          return true;
        }
    }
  }

  return false;
}

BoundState FileList::get_bound_state()
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

void FileList::find_matches(Pcre::Pattern& pattern, bool multiple)
{
  const FileListColumns& columns = filelist_columns();
  long new_sum_matches = 0;

  {
    ScopedBlock block (conn_match_count_);

    for(Gtk::TreeModel::iterator iter = liststore_->children().begin(); iter; ++iter)
    {
      if(signal_pulse()) // emit
        break;

      const FileInfoPtr fileinfo = (*iter)[columns.fileinfo];

      if(!fileinfo->load_failed && !fileinfo->buffer)
        load_file_with_fallback(fileinfo);

      if(fileinfo->load_failed)
        continue;

      g_return_if_fail(fileinfo->buffer);

      ScopedConnection conn (fileinfo->buffer->signal_pulse.connect(signal_pulse.slot()));

      const int n_matches = fileinfo->buffer->find_matches(pattern, multiple);

      if(n_matches > 0)
      {
        if(new_sum_matches == 0)
          path_match_first_ = Gtk::TreePath(iter);

        path_match_last_ = Gtk::TreePath(iter);
      }

      (*iter)[columns.matchcount] = n_matches;
      new_sum_matches += n_matches;
    }

    sum_matches_ = new_sum_matches;
  }

  signal_bound_state_changed(); // emit
  signal_match_count_changed(); // emit
}

long FileList::get_match_count() const
{
  return sum_matches_;
}

void FileList::replace_all_matches(const Glib::ustring& substitution)
{
  const FileListColumns& columns = filelist_columns();
  int new_modified_count = 0;

  {
    ScopedBlock block (conn_match_count_);

    for(Gtk::TreeModel::iterator iter = liststore_->children().begin(); iter; ++iter)
    {
      if(signal_pulse()) // emit
        break;

      const FileInfoPtr fileinfo = (*iter)[columns.fileinfo];
      g_return_if_fail(fileinfo);

      if(fileinfo->buffer)
      {
        ScopedConnection conn (fileinfo->buffer->signal_pulse.connect(signal_pulse.slot()));

        fileinfo->buffer->replace_all_matches(substitution);

        if(fileinfo->buffer->get_modified())
          ++new_modified_count;

        sum_matches_ -= (*iter)[columns.matchcount];
        (*iter)[columns.matchcount] = 0;

        g_return_if_fail(fileinfo->buffer->get_match_count() == 0);
      }
    }
  }

  const bool modified_count_changed = (modified_count_ != new_modified_count);
  modified_count_ = new_modified_count;

  g_return_if_fail(sum_matches_ == 0);

  signal_bound_state_changed(); // emit
  signal_match_count_changed(); // emit

  if(modified_count_changed)
    signal_modified_count_changed(); // emit
}

int FileList::get_modified_count() const
{
  return modified_count_;
}

void FileList::set_fallback_encoding(const std::string& fallback_encoding)
{
  fallback_encoding_ = fallback_encoding;
}

std::string FileList::get_fallback_encoding() const
{
  return fallback_encoding_;
}

/**** Regexxer::FileList -- protected **************************************/

void FileList::on_style_changed(const Glib::RefPtr<Gtk::Style>& previous_style)
{
  color_load_failed_ = get_style()->get_text(Gtk::STATE_INSENSITIVE);

  Gtk::TreeView::on_style_changed(previous_style);
}

/**** Regexxer::FileList -- private ****************************************/

void FileList::cell_data_func(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& iter)
{
  Gtk::CellRendererText *const renderer = static_cast<Gtk::CellRendererText*>(cell);

  if(const FileInfoPtr fileinfo = (*iter)[filelist_columns().fileinfo])
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

  renderer->property_foreground_gdk().reset_value();
}

/** This custom sort function speeds up sorting of huge lists.
 */
int FileList::collatekey_sort_func(const Gtk::TreeModel::iterator& lhs,
                                   const Gtk::TreeModel::iterator& rhs)
{
  const FileListColumns& columns = filelist_columns();

  const std::string lhs_key ((*lhs)[columns.collatekey]);
  const std::string rhs_key ((*rhs)[columns.collatekey]);

  return lhs_key.compare(rhs_key);
}

void FileList::find_recursively(const std::string& dirname, FileList::FindData& find_data)
{
  using namespace Glib;

  Dir dir (dirname);

  for(Dir::iterator pos = dir.begin(); pos != dir.end(); ++pos)
  {
    if(signal_pulse()) // emit
      break;

    const std::string basename (*pos);

    if(!find_data.hidden && *basename.begin() == '.')
      continue;

    const std::string fullname (build_filename(dirname, basename));
    g_assert(fullname.size() > find_data.chop_off);

    try
    {
      if(find_check_file(basename, fullname, find_data))
        find_recursively(fullname, find_data); // recurse
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
}

/* Return value: whether fullname is a subdirectory to be searched recursively.
 * Actually we could call find_recursively() right here in this function, but
 * not doing so reduces the stack depth (on ia32, about 20 bytes per call).
 */
bool FileList::find_check_file(const std::string& basename,
                               const std::string& fullname,
                               FindData& find_data)
{
  using namespace Glib;

  if(file_test(fullname, FILE_TEST_IS_SYMLINK))
    return false;

  if(find_data.recursive && file_test(fullname, FILE_TEST_IS_DIR))
    return true;

  if(file_test(fullname, FILE_TEST_IS_REGULAR) &&
     find_data.pattern.match(Util::filename_to_utf8_fallback(basename)) > 0)
  {
    const FileInfoPtr fileinfo (new FileInfo(fullname));

    const ustring chopped_filename (Util::filename_to_utf8_fallback(
        std::string(fullname, find_data.chop_off, std::string::npos)));

    const FileListColumns& columns = filelist_columns();
    Gtk::TreeModel::Row row (*liststore_->prepend());

    row[columns.filename]   = chopped_filename;
    row[columns.collatekey] = chopped_filename.collate_key();
    row[columns.fileinfo]   = fileinfo;

    ++file_count_;
    signal_file_count_changed(); // emit
  }

  return false;
}

void FileList::on_selection_changed()
{
  const FileListColumns& columns = filelist_columns();

  FileInfoPtr fileinfo;
  int file_index = 0;

  conn_match_count_.disconnect();
  conn_modified_changed_.disconnect();

  if(const Gtk::TreeModel::iterator iter = get_selection()->get_selected())
  {
    Gtk::TreePath path (iter);
    file_index = *path.get_indices().begin() + 1;

    fileinfo = (*iter)[columns.fileinfo];

    if(!fileinfo->buffer)
      load_file_with_fallback(fileinfo);

    if(!fileinfo->load_failed)
    {
      conn_match_count_ = fileinfo->buffer->signal_match_count_changed.
          connect(SigC::slot(*this, &FileList::on_buffer_match_count_changed));

      conn_modified_changed_ = fileinfo->buffer->signal_modified_changed().
          connect(SigC::slot(*this, &FileList::on_buffer_modified_changed));
    }
  }

  signal_switch_buffer(fileinfo, file_index); // emit
  signal_bound_state_changed(); // emit
}

void FileList::on_buffer_match_count_changed(int match_count)
{
  const FileListColumns& columns = filelist_columns();

  // There has to be a selection since we receive signal_match_count_changed()
  // from the currently selected FileBuffer.
  Gtk::TreeModel::iterator iter = get_selection()->get_selected();
  g_return_if_fail(iter);

  const int old_match_count = (*iter)[columns.matchcount];

  if(match_count == old_match_count)
    return; // spurious emission -- do nothing

  const long old_sum_matches = sum_matches_;
  sum_matches_ += (match_count - old_match_count);

  (*iter)[columns.matchcount] = match_count;

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
        do // find the new start boundary of the range
        {
          ++iter;
          g_return_if_fail(iter);
        }
        while((*iter)[columns.matchcount] == 0);

        path_match_first_ = Gtk::TreePath(iter);
      }
      else if(path == path_match_last_)
      {
        do // find the new end boundary of the range
        {
          const bool path_valid = path.prev();
          g_return_if_fail(path_valid);
        }
        while((*liststore_->get_iter(path))[columns.matchcount] == 0);

        path_match_last_ = path;
      }
    }

    signal_bound_state_changed(); // emit
  }

  signal_match_count_changed(); // emit
}

void FileList::on_buffer_modified_changed()
{
  const Gtk::TreeModel::iterator iter = get_selection()->get_selected();
  g_return_if_fail(iter);

  const FileInfoPtr fileinfo = (*iter)[filelist_columns().fileinfo];
  g_return_if_fail(fileinfo && fileinfo->buffer);

  if(fileinfo->buffer->get_modified())
    ++modified_count_;
  else
    --modified_count_;

  g_return_if_fail(modified_count_ >= 0);

  liststore_->row_changed(Gtk::TreePath(iter), iter);
  signal_modified_count_changed(); // emit
}

void FileList::load_file_with_fallback(const FileInfoPtr& fileinfo)
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

Glib::RefPtr<FileBuffer> FileList::create_error_message_buffer(const Glib::ustring& message)
{
  if(!error_pixbuf_)
    error_pixbuf_ = render_icon(Gtk::Stock::DIALOG_ERROR, Gtk::ICON_SIZE_DIALOG);

  return FileBuffer::create_with_error_message(error_pixbuf_, message);
}

} // namespace Regexxer

