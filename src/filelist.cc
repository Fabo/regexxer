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

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <gtkmm/liststore.h>
#include <gtkmm/textbuffer.h>


namespace
{

enum
{
  BUFSIZE               = 4096,
  FIND_UPDATE_INTERVAL  = 16
};

const char fallback_encoding[] = "ISO-8859-1";

struct FileListColumns : public Gtk::TreeModel::ColumnRecord
{
  Gtk::TreeModelColumn<Glib::ustring>         filename;
  Gtk::TreeModelColumn<int>                   matchcount;
  Gtk::TreeModelColumn<Regexxer::FileInfoPtr> fileinfo;

  FileListColumns() { add(filename); add(matchcount); add(fileinfo); }
};

const FileListColumns& filelist_columns() G_GNUC_CONST;
const FileListColumns& filelist_columns()
{
  static FileListColumns column_record;
  return column_record;
}

} // anonymous namespace


namespace Regexxer
{

/**** Regexxer::FileInfo ***************************************************/

FileInfo::FileInfo(const std::string& fullname_)
:
  fullname    (fullname_),
  load_failed (false)
{}

FileInfo::~FileInfo()
{}


/**** Regexxer::FileList::FindData *****************************************/

struct FileList::FindData
{
  FindData(const Glib::ustring&   pattern_,
           std::string::size_type chop_off_,
           bool recursive_, bool hidden_);
  ~FindData();

  Pcre::Pattern                 pattern;
  const std::string::size_type  chop_off;
  const bool                    recursive;
  const bool                    hidden;
  unsigned int                  iteration;
};

FileList::FindData::FindData(const Glib::ustring&   pattern_,
                             std::string::size_type chop_off_,
                             bool recursive_, bool hidden_)
:
  pattern   (pattern_),
  chop_off  (chop_off_),
  recursive (recursive_),
  hidden    (hidden_),
  iteration (0)
{}

FileList::FindData::~FindData()
{}


/**** Regexxer::FileList ***************************************************/

FileList::FileList()
:
  liststore_      (Gtk::ListStore::create(filelist_columns())),
  color_modified_ ("red"),
  find_running_   (false),
  find_stop_      (false),
  sum_matches_    (0),
  modified_count_ (0)
{
  set_model(liststore_);

  const FileListColumns& columns = filelist_columns();

  append_column("File", columns.filename);
  append_column("#",    columns.matchcount);

  Gtk::TreeView::Column *const file_column = get_column(0);
  file_column->set_cell_data_func(*file_column->get_first_cell_renderer(),
                                  SigC::slot(*this, &FileList::cell_data_func));
  file_column->set_resizable(true);
  set_search_column(0);

  Gtk::TreeView::Column *const count_column = get_column(1);
  count_column->set_cell_data_func(*count_column->get_first_cell_renderer(),
                                   SigC::slot(*this, &FileList::cell_data_func));
  count_column->set_alignment(1.0);
  count_column->get_first_cell_renderer()->property_xalign() = 1.0;

  liststore_->set_sort_column_id(columns.filename, Gtk::SORT_ASCENDING);

  get_selection()->signal_changed().connect(SigC::slot(*this, &FileList::on_selection_changed));
}

FileList::~FileList()
{}

void FileList::find_files(const Glib::ustring& dirname,
                          const Glib::ustring& pattern,
                          bool recursive, bool hidden)
{
  if(find_running_)
  {
    stop_find_files();
    return;
  }

  const std::string startdir = Glib::filename_from_utf8(dirname);
  std::string::size_type chop_off = startdir.length();

  if(chop_off > 0 && *startdir.rbegin() != G_DIR_SEPARATOR)
    ++chop_off;

  const bool modified_count_changed = (modified_count_ != 0);

  get_selection()->unselect_all(); // workaround for GTK+ <= 2.0.6 (#94868)
  liststore_->clear();

  sum_matches_    = 0;
  modified_count_ = 0;

  signal_bound_state_changed(); // emit
  signal_match_count_changed(); // emit

  if(modified_count_changed)
    signal_modified_count_changed(); // emit

  find_stop_    = false;
  find_running_ = true;

  try
  {
    FindData find_data (Util::shell_pattern_to_regex(pattern), chop_off, recursive, hidden);
    find_recursively(startdir, find_data);
  }
  catch(const Pcre::Error& e)
  {
    std::cerr << e.what() << std::endl;
  }

  find_running_ = false;

  signal_bound_state_changed(); // emit
}

void FileList::stop_find_files()
{
  find_stop_ = true;
}

void FileList::save_current_file()
{
  if(Gtk::TreeModel::iterator iter = get_selection()->get_selected())
  {
    const FileInfoPtr fileinfo = (*iter)[filelist_columns().fileinfo];

    try
    {
      if(fileinfo->buffer)
        save_file(fileinfo);
    }
    catch(...)
    {
      const Glib::ustring filename = Util::filename_to_utf8_fallback(fileinfo->fullname);
      g_warning("writing to file `%s' failed", filename.c_str());
    }
  }
}

void FileList::save_all_files()
{
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
      catch(...)
      {
        const Glib::ustring filename = Util::filename_to_utf8_fallback(fileinfo->fullname);
        g_warning("writing to file `%s' failed", filename.c_str());
      }

      liststore_->row_changed(Gtk::TreePath(iter), iter);
    }
  }

  if(modified_count_ != new_modified_count)
  {
    modified_count_ = new_modified_count;
    signal_modified_count_changed(); // emit
  }
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

  conn_match_count_.block();

  for(Gtk::TreeModel::iterator iter = liststore_->children().begin(); iter; ++iter)
  {
    const FileInfoPtr fileinfo = (*iter)[columns.fileinfo];

    if(!fileinfo->buffer)
    {
      load_file(fileinfo);

      if(!fileinfo->buffer)
        continue;
    }

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

  conn_match_count_.unblock();

  signal_bound_state_changed(); // emit
  signal_match_count_changed(); // emit
}

long FileList::get_match_count() const
{
  return sum_matches_;
}

void FileList::replace_all_matches(const Glib::ustring& substitution)
{
  const Glib::RefPtr<Glib::MainContext> main_context = Glib::MainContext::get_default();
  const FileListColumns& columns = filelist_columns();
  int new_modified_count = 0;

  conn_match_count_.block();

  for(Gtk::TreeModel::iterator iter = liststore_->children().begin(); iter; ++iter)
  {
    while(main_context->iteration(false)) {}

    const FileInfoPtr fileinfo = (*iter)[columns.fileinfo];
    g_return_if_fail(fileinfo);

    if(fileinfo->buffer)
    {
      fileinfo->buffer->replace_all_matches(substitution);

      if(fileinfo->buffer->get_modified())
        ++new_modified_count;

      sum_matches_ -= (*iter)[columns.matchcount];
      (*iter)[columns.matchcount] = 0;

      g_return_if_fail(fileinfo->buffer->get_match_count() == 0);
    }
  }

  const bool modified_count_changed = (modified_count_ != new_modified_count);
  modified_count_ = new_modified_count;

  conn_match_count_.unblock();

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

/**** Regexxer::FileList -- private ****************************************/

void FileList::cell_data_func(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& iter)
{
  Gtk::CellRendererText *const renderer = dynamic_cast<Gtk::CellRendererText*>(cell);

  g_return_if_fail(renderer != 0);
  g_return_if_fail(iter);

  if(const FileInfoPtr fileinfo = (*iter)[filelist_columns().fileinfo])
  {
    if(fileinfo->load_failed)
    {
      renderer->property_foreground_gdk() = get_style()->get_text(Gtk::STATE_INSENSITIVE);
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

void FileList::find_recursively(const std::string& dirname, FileList::FindData& find_data)
{
  using namespace Glib;

  Dir dir (dirname);

  for(Dir::iterator pos = dir.begin(); pos != dir.end(); ++pos)
  {
    if((++find_data.iteration % FIND_UPDATE_INTERVAL) == 0)
    {
      const RefPtr<MainContext> main_context = MainContext::get_default();
      while(!find_stop_ && main_context->iteration(false)) {}

      if(find_stop_)
        break;
    }

    const std::string basename = *pos;

    if(!find_data.hidden && *basename.begin() == '.')
      continue;

    const std::string fullname (build_filename(dirname, basename));
    g_assert(fullname.size() > find_data.chop_off);

    if(file_test(fullname, FILE_TEST_IS_SYMLINK))
      continue;

    if(find_data.recursive && file_test(fullname, FILE_TEST_IS_DIR))
    {
      find_recursively(fullname, find_data);
    }
    else if(file_test(fullname, FILE_TEST_IS_REGULAR))
    {
      try
      {
        if(find_data.pattern.match(Util::filename_to_utf8_fallback(basename)) > 0)
        {
          const FileInfoPtr fileinfo (new FileInfo(fullname));
          const ustring chopped_filename = Util::filename_to_utf8_fallback(
              std::string(fullname, find_data.chop_off, std::string::npos));

          const FileListColumns& columns = filelist_columns();
          Gtk::TreeModel::Row row = *liststore_->append();

          row[columns.filename] = chopped_filename;
          row[columns.fileinfo] = fileinfo;
        }
      }
      catch(const Glib::ConvertError& error)
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
}

void FileList::on_selection_changed()
{
  const FileListColumns& columns = filelist_columns();
  FileInfoPtr fileinfo;

  conn_match_count_.disconnect();
  conn_modified_changed_.disconnect();

  if(Gtk::TreeModel::iterator iter = get_selection()->get_selected())
  {
    fileinfo = (*iter)[columns.fileinfo];

    if(!fileinfo->buffer)
      load_file(fileinfo);

    if(fileinfo->buffer)
    {
      conn_match_count_ = fileinfo->buffer->signal_match_count_changed.
          connect(SigC::slot(*this, &FileList::on_buffer_match_count_changed));

      conn_modified_changed_ = fileinfo->buffer->signal_modified_changed().
          connect(SigC::slot(*this, &FileList::on_buffer_modified_changed));
    }
  }

  signal_switch_buffer(fileinfo); // emit
  signal_bound_state_changed();   // emit
}

void FileList::on_buffer_match_count_changed(int match_count)
{
  const FileListColumns& columns = filelist_columns();

  Gtk::TreeModel::iterator iter = get_selection()->get_selected();
  g_return_if_fail(iter);

  const int old_match_count = (*iter)[columns.matchcount];

  if(match_count == old_match_count)
    return;

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

    g_return_if_fail(path_match_first_ < path_match_last_);

    Gtk::TreePath path (iter);

    if(old_match_count == 0) // implies match_count > 0
    {
      // Expand the range if necessary.
      if(path < path_match_first_)
        path_match_first_ = path;
      else if(path > path_match_last_)
        path_match_last_ = path;
    }
    else if(path == path_match_first_) // implies match_count == 0
    {
      // Find the new start boundary of the range.
      do
      {
        ++iter;
        g_return_if_fail(iter);
      }
      while((*iter)[columns.matchcount] == 0);

      path_match_first_ = Gtk::TreePath(iter);
    }
    else if(path == path_match_last_) // implies match_count == 0
    {
      // Find the new end boundary of the range.
      do
      {
        const bool path_valid = path.prev();
        g_return_if_fail(path_valid);
      }
      while((*liststore_->get_iter(path))[columns.matchcount] == 0);

      path_match_last_ = path;
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

void FileList::load_file(const Util::SharedPtr<FileInfo>& fileinfo)
{
  std::ifstream input_stream;
  input_stream.exceptions(std::ios_base::badbit);
  input_stream.open(fileinfo->fullname.c_str(), std::ios::in | std::ios::binary);

  std::string encoding = "UTF-8";
  Glib::RefPtr<FileBuffer> buffer = load_stream(input_stream);

  if(!buffer)
  {
    if(!Glib::get_charset(encoding)) // locale charset is _not_ UTF-8
    {
      input_stream.clear();
      input_stream.seekg(0);
      Glib::IConv iconv ("UTF-8", encoding);
      buffer = load_convert_stream(input_stream, iconv);
    }
    if(!buffer && !Util::encodings_equal(encoding, fallback_encoding))
    {
      input_stream.clear();
      input_stream.seekg(0);
      encoding = fallback_encoding;
      Glib::IConv iconv ("UTF-8", encoding);
      buffer = load_convert_stream(input_stream, iconv);
    }
    if(!buffer)
    {
      fileinfo->load_failed = true;
      std::cerr << "Couldn't convert `"
                << Util::filename_to_utf8_fallback(fileinfo->fullname)
                << "' to UTF-8.\n";
      return;
    }
  }

  buffer->set_modified(false);
  fileinfo->load_failed = false;
  fileinfo->encoding    = encoding;
  fileinfo->buffer      = buffer;
}

Glib::RefPtr<FileBuffer> FileList::load_stream(std::istream& input)
{
  const Glib::RefPtr<FileBuffer> text_buffer = FileBuffer::create();

  const Glib::ScopedPtr<char> inbuf (g_new(char, BUFSIZE));
  size_t length_incomplete = 0;

  while(input)
  {
    input.read(inbuf.get() + length_incomplete, BUFSIZE - length_incomplete);
    const size_t n_read = input.gcount();

    // For now, let's assume that any invalid UTF-8 in the input is
    // just an incomplete character at the end of the buffer.
    const char* start_incomplete = 0;
    g_utf8_validate(inbuf.get(), length_incomplete + n_read, &start_incomplete);
    length_incomplete = (inbuf.get() + length_incomplete + n_read) - start_incomplete;

    // If the remaining buffer space after the valid area is >= 6 bytes (the
    // maximum length of a single UTF-8 character), it can't in fact be just
    // an incomplete sequence.  Report failure.
    if(length_incomplete >= 6)
      return Glib::RefPtr<FileBuffer>();

    // Insert all the valid stuff into the text buffer.
    text_buffer->insert(text_buffer->end(), inbuf.get(), start_incomplete);

    // Move the trailing invalid sequence to the front.
    std::memcpy(inbuf.get(), start_incomplete, length_incomplete);
  }

  // At the end of the file there shouldn't be any invalid sequence left.
  if(length_incomplete > 0)
    return Glib::RefPtr<FileBuffer>();

  return text_buffer;
}

Glib::RefPtr<FileBuffer> FileList::load_convert_stream(std::istream& input, Glib::IConv& iconv)
{
  const Glib::RefPtr<FileBuffer> text_buffer = FileBuffer::create();

  const Glib::ScopedPtr<char> inbuf   (g_new(char, BUFSIZE));
  const Glib::ScopedPtr<char> convbuf (g_new(char, BUFSIZE));

  gsize inleft = 0;

  while(input)
  {
    // inleft might be > 0 if there was an incomplete
    // multibyte sequence at the end of inbuf.
    input.read(inbuf.get() + inleft, BUFSIZE - inleft);
    inleft += input.gcount();

    char* inpos    = inbuf.get();
    char* convpos  = convbuf.get();
    gsize convleft = BUFSIZE;

    while(iconv.iconv(&inpos, &inleft, &convpos, &convleft) == static_cast<size_t>(-1))
    {
      const int err_no = errno;

      if(err_no == E2BIG)
      {
        // The last character written to convbuf might be incomplete.
        char *const start_incomplete = g_utf8_find_prev_char(convbuf.get(), convpos);
        g_assert(start_incomplete != 0);

        // Gtk::TextBuffer doesn't like '\0' characters.
        if(Util::contains_null(convbuf.get(), start_incomplete))
          return Glib::RefPtr<FileBuffer>();

        // Append what we have so far.
        text_buffer->insert(text_buffer->end(), convbuf.get(), start_incomplete);

        // Move the trailing incomplete sequence to the front.
        const size_t length_incomplete = convpos - start_incomplete;
        std::memcpy(convbuf.get(), start_incomplete, length_incomplete);

        convpos  = convbuf.get() + length_incomplete;
        convleft = BUFSIZE - length_incomplete;
      }
      else if(err_no == EINVAL)
      {
        // Move the trailing incomplete sequence to the front.
        std::memcpy(inbuf.get(), inpos, inleft);
        break;
      }
      else
      {
        if(err_no != EILSEQ)
          g_error("unexpected IConv error: %s", g_strerror(err_no));

        return Glib::RefPtr<FileBuffer>();
      }
    }

    // Gtk::TextBuffer doesn't like '\0' characters.
    if(Util::contains_null(convbuf.get(), convpos))
      return Glib::RefPtr<FileBuffer>();

    // Append what we have so far.
    text_buffer->insert(text_buffer->end(), convbuf.get(), convpos);
  }

  // At the end of the file there shouldn't be any data left.
  if(inleft > 0)
    return Glib::RefPtr<FileBuffer>();

  return text_buffer;
}

void FileList::save_file(const Util::SharedPtr<FileInfo>& fileinfo)
{
  std::ofstream output_stream;
  output_stream.exceptions(std::ios_base::badbit | std::ios::failbit);
  output_stream.open(fileinfo->fullname.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);

  if(Util::encodings_equal(fileinfo->encoding, "UTF-8"))
  {
    save_stream(output_stream, fileinfo->buffer);
  }
  else
  {
    Glib::IConv iconv (fileinfo->encoding, "UTF-8");
    save_convert_stream(output_stream, iconv, fileinfo->buffer);
  }

  output_stream.close(); // might throw

  fileinfo->buffer->set_modified(false);
}

void FileList::save_stream(std::ostream& output, const Glib::RefPtr<FileBuffer>& buffer)
{
  FileBuffer::iterator start = buffer->begin();
  FileBuffer::iterator stop  = start;

  for(; start; start = stop)
  {
    stop.forward_chars(BUFSIZE); // inaccurate, but doesn't matter
    const Glib::ustring chunk (buffer->get_text(start, stop));

    output.write(chunk.data(), chunk.bytes());
  }
}

void FileList::save_convert_stream(std::ostream& output, Glib::IConv& iconv,
                                   const Glib::RefPtr<FileBuffer>& buffer)
{
  const Glib::ScopedPtr<char> convbuf (g_new(char, BUFSIZE));

  FileBuffer::iterator start = buffer->begin();
  FileBuffer::iterator stop  = start;

  for(; start; start = stop)
  {
    stop.forward_chars(BUFSIZE); // inaccurate, but doesn't matter
    const Glib::ustring chunk (buffer->get_text(start, stop));

    char* inpos    = const_cast<char*>(chunk.data());
    gsize inleft   = chunk.bytes();
    char* convpos  = convbuf.get();
    gsize convleft = BUFSIZE;

    while(iconv.iconv(&inpos, &inleft, &convpos, &convleft) == static_cast<size_t>(-1))
    {
      const int err_no = errno;

      if(err_no != E2BIG)
        g_error("unexpected IConv error: %s", g_strerror(err_no));

      output.write(convbuf.get(), BUFSIZE - convleft);

      convpos  = convbuf.get();
      convleft = BUFSIZE;
    }

    output.write(convbuf.get(), BUFSIZE - convleft);
  }
}

} // namespace Regexxer

