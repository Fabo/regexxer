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

#include "filebuffer.h"
#include "filebufferundo.h"
#include "pcreshell.h"
#include "stringutils.h"

#include <glib.h>
#include <algorithm>
#include <list>


namespace
{

enum { PULSE_INTERVAL = 128 };


class RegexxerTags : public Gtk::TextTagTable
{
public:
  typedef Glib::RefPtr<Gtk::TextTag> TextTagPtr;

  // This is a global singleton shared by all FileBuffer instances.
  static const Glib::RefPtr<RegexxerTags>& instance() G_GNUC_CONST;

  TextTagPtr error_message;
  TextTagPtr error_title;
  TextTagPtr match;
  TextTagPtr current;

protected:
  RegexxerTags();
  virtual ~RegexxerTags();
};

RegexxerTags::RegexxerTags()
:
  error_message (Gtk::TextTag::create("regexxer-error-message")),
  error_title   (Gtk::TextTag::create("regexxer-error-title")),
  match         (Gtk::TextTag::create("regexxer-match")),
  current       (Gtk::TextTag::create("regexxer-current-match"))
{
  error_message->property_wrap_mode()          = Gtk::WRAP_WORD;
  error_message->property_justification()      = Gtk::JUSTIFY_CENTER;
  error_message->property_pixels_above_lines() = 10;

  error_title->property_scale() = Pango::SCALE_X_LARGE;

  match  ->property_background() = "orange";
  current->property_background() = "yellow";

  add(error_message);
  add(error_title);
  add(match);
  add(current);
}

RegexxerTags::~RegexxerTags()
{}

// static
const Glib::RefPtr<RegexxerTags>& RegexxerTags::instance()
{
  static Glib::RefPtr<RegexxerTags> global_table (new RegexxerTags());
  return global_table;
}

} // anonymous namespace


namespace Regexxer
{

/**** Regexxer::FileBuffer::ScopedLock *************************************/

class FileBuffer::ScopedLock
{
private:
  FileBuffer& buffer_;

  ScopedLock(const FileBuffer::ScopedLock&);
  FileBuffer::ScopedLock& operator=(const FileBuffer::ScopedLock&);

public:
  explicit ScopedLock(FileBuffer& buffer);
  ~ScopedLock();
};

FileBuffer::ScopedLock::ScopedLock(FileBuffer& buffer)
:
  buffer_ (buffer)
{
  g_return_if_fail(!buffer_.locked_);
  buffer_.locked_ = true;
}

FileBuffer::ScopedLock::~ScopedLock()
{
  g_return_if_fail(buffer_.locked_);
  buffer_.locked_ = false;
}


/**** Regexxer::FileBuffer::ScopedUserAction *******************************/

class FileBuffer::ScopedUserAction
{
private:
  FileBuffer& buffer_;

  ScopedUserAction(const FileBuffer::ScopedUserAction&);
  FileBuffer::ScopedUserAction& operator=(const FileBuffer::ScopedUserAction&);

public:
  explicit ScopedUserAction(FileBuffer& buffer)
    : buffer_ (buffer) { buffer_.begin_user_action(); }

  ~ScopedUserAction() { buffer_.end_user_action(); }
};


/**** Regexxer::FileBuffer *************************************************/

FileBuffer::FileBuffer()
:
  Gtk::TextBuffer       (RegexxerTags::instance()),
  match_set_            (),
  match_count_          (0),
  original_match_count_ (0),
  current_match_        (match_set_.end()),
  match_removed_        (false),
  bound_state_          (BOUND_FIRST | BOUND_LAST),
  locked_               (false),
  user_action_stack_    (),
  stamp_modified_       (0),
  stamp_saved_          (0)
{}

FileBuffer::~FileBuffer()
{}

// static
Glib::RefPtr<FileBuffer> FileBuffer::create()
{
  return Glib::RefPtr<FileBuffer>(new FileBuffer());
}

// static
Glib::RefPtr<FileBuffer>
FileBuffer::create_with_error_message(const Glib::RefPtr<Gdk::Pixbuf>& pixbuf,
                                      const Glib::ustring& message)
{
  const Glib::RefPtr<FileBuffer> buffer (new FileBuffer());

  const Glib::RefPtr<RegexxerTags>& tagtable = RegexxerTags::instance();
  iterator pend = buffer->end();

  // "\302\240" == no-break space
  const Glib::ustring title = "\302\240Can't\302\240read\302\240file:";

  pend = buffer->insert_pixbuf(pend, pixbuf);
  pend = buffer->insert_with_tag(pend, title, tagtable->error_title);
  pend = buffer->insert(pend, "\n");
  pend = buffer->insert(pend, message);

  if(!message.empty() && *message.rbegin() != '.')
    pend = buffer->insert(pend, ".");

  buffer->apply_tag(tagtable->error_message, buffer->begin(), pend);
  buffer->set_modified(false);

  return buffer;
}

// static
void FileBuffer::pango_context_changed(const Glib::RefPtr<Pango::Context>& context)
{
  // This magic code calculates the height to rise the error message title,
  // so that it's displayed approximately in line with the error icon. By
  // default the text would appear at the bottom, and since the icon is
  // about 48 pixels tall this looks incredibly ugly.

  int font_size = context->get_font_description().get_size();

  if(font_size <= 0) // urgh, fall back to some reasonable value
    font_size = 10 * Pango::SCALE;

  int icon_width = 0, icon_height = 0;
  Gtk::IconSize::lookup(Gtk::ICON_SIZE_DIALOG, icon_width, icon_height);

  g_return_if_fail(icon_height > 0); // the lookup should never fail for builtin icon sizes

  const int title_size  = int(Pango::SCALE_X_LARGE * font_size);
  const int rise_height = (icon_height * Pango::SCALE - title_size) / 2;

  const Glib::RefPtr<RegexxerTags>& tagtable = RegexxerTags::instance();

  tagtable->error_title->property_rise() = rise_height;
}

bool FileBuffer::is_freeable() const
{
  return (!locked_ && match_count_ == 0 && !get_modified());
}

bool FileBuffer::in_user_action() const
{
  return user_action_stack_;
}

/* Apply pattern on all lines in the buffer and return the number of matches.
 * If multiple is false then every line is matched only once, otherwise
 * multiple matches per line will be found (like modifier /g in Perl).
 */
int FileBuffer::find_matches(Pcre::Pattern& pattern, bool multiple)
{
  ScopedLock lock (*this);

  const Glib::RefPtr<Glib::MainContext> main_context = Glib::MainContext::get_default();
  const Glib::RefPtr<RegexxerTags>& tagtable = RegexxerTags::instance();

  forget_current_match();
  remove_tag(tagtable->match, begin(), end());

  while(!match_set_.empty())
    delete_mark((*match_set_.begin())->mark);

  g_return_val_if_fail(match_count_ == 0, 0);
  original_match_count_ = 0;

  unsigned int iteration = 0;

  for(iterator line = begin(); !line.is_end(); line.forward_line())
  {
    iterator line_end (line);

    if(!line_end.ends_line())
      line_end.forward_to_line_end();

    const Glib::ustring subject (get_slice(line, line_end));
    int  offset = 0;
    bool allow_empty_match = true;

    do
    {
      if((++iteration % PULSE_INTERVAL) == 0 && signal_pulse()) // emit
      {
        signal_match_count_changed(match_count_); // emit
        return match_count_;
      }

      const int capture_count = pattern.match(
          subject, offset, (allow_empty_match) ? Pcre::MatchOptions(0) : Pcre::NOT_EMPTY);

      if(capture_count <= 0)
      {
        if(!allow_empty_match && unsigned(offset) < subject.bytes())
        {
          const std::string::const_iterator pbegin = subject.begin().base();
          Glib::ustring::const_iterator     poffset (pbegin + offset);

          offset = (++poffset).base() - pbegin; // forward one UTF-8 character
          allow_empty_match = true;
          continue;
        }
        break;
      }

      ++match_count_;
      ++original_match_count_;

      const std::pair<int,int> bounds (pattern.get_substring_bounds(0));

      iterator start (line);
      iterator stop  (line);

      start.set_line_index(bounds.first);
      stop .set_line_index(bounds.second);

      const MatchDataPtr match (new MatchData(
          original_match_count_, subject, pattern, capture_count));

      match_set_.insert(match_set_.end(), match);
      match->install_mark(start);

      apply_tag(tagtable->match, start, stop);

      allow_empty_match = (bounds.second > bounds.first);
      offset = bounds.second;
    }
    while(multiple);
  }

  signal_match_count_changed(match_count_); // emit
  return match_count_;
}

int FileBuffer::get_match_count() const
{
  return match_count_;
}

int FileBuffer::get_match_index() const
{
  // Stupid work-around for silly, silly gcc 2.95.x.
  const MatchSet::const_iterator current_match (current_match_);

  return (!match_removed_ && current_match != match_set_.end()) ? (*current_match_)->index : 0;
}

int FileBuffer::get_original_match_count() const
{
  return original_match_count_;
}

/* Move to the next match in the buffer.  If there is a next match
 * its position will be returned, otherwise 0.
 */
Glib::RefPtr<FileBuffer::Mark> FileBuffer::get_next_match(bool move_forward)
{
  remove_tag_current();

  // If we're called just after a removal, then current_match_ already points
  // to the next match but it doesn't have the "current-match" tag set.  So we
  // prevent moving forward since this would cause one match to be skipped.
  // Moving backward is OK though.

  if(move_forward && !match_removed_)
  {
    if(current_match_ != match_set_.end())
      ++current_match_;
    else
      current_match_ = match_set_.begin();
  }

  if(!move_forward)
  {
    if(current_match_ != match_set_.begin())
      --current_match_;
    else
      current_match_ = match_set_.end();
  }

  // See above; we can now safely reset this flag.
  match_removed_ = false;

  apply_tag_current();
  update_bound_state();

  return (current_match_ != match_set_.end()) ? (*current_match_)->mark : Glib::RefPtr<Mark>();
}

/* Remove the highlight tag from the currently selected match and forget
 * about it.  The next call to get_next_match() will start over at the
 * first respectively last match in the buffer.
 */
void FileBuffer::forget_current_match()
{
  remove_tag_current();
  current_match_ = match_set_.end();
  match_removed_ = false;
}

BoundState FileBuffer::get_bound_state()
{
  bound_state_ = BOUND_NONE;

  if(match_set_.empty())
  {
    bound_state_ = BOUND_FIRST | BOUND_LAST;
  }
  else if(current_match_ != match_set_.end())
  {
    if(current_match_ == match_set_.begin())
      bound_state_ |= BOUND_FIRST;

    if(!match_removed_ && current_match_ == --match_set_.end())
      bound_state_ |= BOUND_LAST;
  }

  return bound_state_;
}

/* Replace the currently selected match with substitution.  References
 * to captured substrings in substitution will be interpolated.  This
 * method indirectly triggers emission of signal_match_count_changed()
 * and signal_preview_line_changed().
 */
void FileBuffer::replace_current_match(const Glib::ustring& substitution)
{
  if(!match_removed_ && current_match_ != match_set_.end())
  {
    ScopedUserAction action (*this);

    replace_match(current_match_, substitution);
  }
}

void FileBuffer::replace_all_matches(const Glib::ustring& substitution)
{
  ScopedLock lock (*this);
  ScopedUserAction action (*this);

  unsigned int iteration = 0;

  while(!match_set_.empty())
  {
    replace_match(match_set_.begin(), substitution);

    if((++iteration % PULSE_INTERVAL) == 0 && signal_pulse()) // emit
      break;
  }
}

/* Build a preview of what replace_current_match() would do to the current
 * line if it were called with substitution as argument.  References to
 * captured substrings in substitution will be interpolated.  The result is
 * written to the output argument preview.  The return value is a character
 * offset into preview pointing to the end of the replaced text.
 */
int FileBuffer::get_line_preview(const Glib::ustring& substitution, Glib::ustring& preview)
{
  int position = -1;

  if(!match_removed_ && current_match_ != match_set_.end())
  {
    // Get the start of the match.
    const iterator start ((*current_match_)->mark->get_iter());

    // Find the end of the match.
    iterator stop (start);
    stop.forward_chars((*current_match_)->get_match_length());

    // Find begin and end of the line containing the match.
    iterator line_begin (start);
    iterator line_end   (stop);
    find_line_bounds(line_begin, line_end);

    const std::string& subject = (*current_match_)->subject.raw();

    // Construct the preview line: [line_begin,start) + substitution + [stop,line_end)
    preview  = get_text(line_begin, start);
    preview += Util::substitute_references(substitution.raw(), subject, (*current_match_)->captures);
    position = preview.length();
    preview += get_text(stop, line_end);
  }

  return position;
}

void FileBuffer::increment_stamp()
{
  ++stamp_modified_;

  // No need to call set_modified() since the buffer has been modified
  // already by the action that caused the stamp to increment.
}

void FileBuffer::decrement_stamp()
{
  g_return_if_fail(stamp_modified_ > 0);

  --stamp_modified_;

  set_modified(stamp_modified_ != stamp_saved_);
}

void FileBuffer::undo_remove_match(const MatchDataPtr& match, int offset)
{
  const std::pair<MatchSet::iterator,bool> pos = match_set_.insert(match);
  g_return_if_fail(pos.second);

  const iterator start (get_iter_at_offset(offset));
  match->install_mark(start);

  if(match->get_match_length() > 0)
  {
    const Glib::RefPtr<RegexxerTags>& tagtable = RegexxerTags::instance();

    iterator stop (start);
    stop.forward_chars(match->get_match_length());

    apply_tag(tagtable->match, start, stop);
  }

  if(match_removed_ && Util::prior(current_match_) == pos.first)
  {
    remove_tag_current();

    current_match_ = pos.first;
    match_removed_ = false;

    apply_tag_current();
  }

  signal_preview_line_changed.queue();

  signal_match_count_changed(++match_count_); // emit
  update_bound_state();
}

/**** Regexxer::FileBuffer -- protected ************************************/

void FileBuffer::on_insert(const FileBuffer::iterator& pos, const Glib::ustring& text, int bytes)
{
  if(!text.empty())
  {
    if(!match_removed_ && current_match_ != match_set_.end())
    {
      // Test whether pos is within the current match and push
      // signal_preview_line_changed() on the queue if true.

      iterator lbegin ((*current_match_)->mark->get_iter());
      iterator lend   (lbegin);
      find_line_bounds(lbegin, lend);

      if(pos.in_range(lbegin, lend))
        signal_preview_line_changed.queue();
    }

    const Glib::RefPtr<RegexxerTags>& tagtable = RegexxerTags::instance();

    // If pos is within a match then remove this match.
    if(pos.has_tag(tagtable->match) && !is_match_start(pos))
    {
      iterator start (pos);
      start.backward_to_tag_toggle(tagtable->match);
      remove_match_at_iter(start);
    }
  }

  if(user_action_stack_)
  {
    user_action_stack_->push(UndoActionPtr(
        new FileBufferActionInsert(*this, pos.get_offset(), text)));
  }

  Gtk::TextBuffer::on_insert(pos, text, bytes);
}

void FileBuffer::on_erase(const FileBuffer::iterator& rbegin, const FileBuffer::iterator& rend)
{
  if(!match_removed_ && current_match_ != match_set_.end())
  {
    // Test whether [rbegin,rend) overlaps with the current match
    // and push signal_preview_line_changed() on the queue if true.

    iterator lbegin ((*current_match_)->mark->get_iter());
    iterator lend   (lbegin);
    find_line_bounds(lbegin, lend);

    if(lbegin.in_range(rbegin, rend) || rbegin.in_range(lbegin, lend))
      signal_preview_line_changed.queue();
  }

  const Glib::RefPtr<RegexxerTags>& tagtable = RegexxerTags::instance();

  if(!rbegin.starts_line() && rbegin.has_tag(tagtable->match))
  {
    g_return_if_fail(!rbegin.ends_tag(tagtable->match)); // just to be sure...

    int backward_chars = 0;
    int match_length = -1;
    iterator pos (rbegin);

    do
    {
      ++backward_chars;
      --pos;

      typedef std::list< Glib::RefPtr<Mark> > MarkList;
      const MarkList marks (pos.get_marks());

      for(MarkList::const_iterator pmark = marks.begin(); pmark != marks.end(); ++pmark)
      {
        if(const MatchDataPtr match_data = MatchData::get_from_mark(*pmark))
          match_length = std::max(match_length, match_data->get_match_length());
      }
    }
    while(match_length < 0 && !pos.starts_line() && !pos.toggles_tag(tagtable->match));

    if(match_length > backward_chars)
      remove_match_at_iter(pos);
  }

  for(iterator pos = rbegin; pos != rend; ++pos)
    remove_match_at_iter(pos);

  if(user_action_stack_)
  {
    user_action_stack_->push(UndoActionPtr(
        new FileBufferActionErase(*this, rbegin.get_offset(), get_slice(rbegin, rend))));
  }

  Gtk::TextBuffer::on_erase(rbegin, rend);
}

void FileBuffer::on_mark_deleted(const Glib::RefPtr<TextBuffer::Mark>& mark)
{
  Gtk::TextBuffer::on_mark_deleted(mark);

  // If a mark has been deleted we check whether it was one of our match
  // marks and if so, remove it from the list.  Deletion of the current
  // match has to be special cased, but's also a bit faster this way since
  // it's the most common situation and we don't need to traverse the list.

  if(current_match_ != match_set_.end() && (*current_match_)->mark == mark)
  {
    const MatchSet::iterator pos = current_match_++;

    (*pos)->mark.clear();
    match_set_.erase(pos);
    match_removed_ = true;

    signal_match_count_changed(--match_count_); // emit
    update_bound_state();
  }
  else if(const MatchDataPtr match_data = MatchData::get_from_mark(mark))
  {
    const MatchSet::iterator pos = match_set_.find(match_data);

    if(pos != match_set_.end())
    {
      (*pos)->mark.clear();
      match_set_.erase(pos);

      signal_match_count_changed(--match_count_); // emit
      update_bound_state();
    }
  }
}

void FileBuffer::on_modified_changed()
{
  if(!get_modified())
    stamp_saved_ = stamp_modified_;
}

void FileBuffer::on_begin_user_action()
{
  g_return_if_fail(!user_action_stack_);

  user_action_stack_.reset(new UndoStack());
}

void FileBuffer::on_end_user_action()
{
  g_return_if_fail(user_action_stack_);

  const UndoStackPtr undo_action (user_action_stack_);
  user_action_stack_.reset();

  if(!undo_action->empty())
    signal_undo_stack_push(undo_action); // emit
}

/**** Regexxer::FileBuffer -- private **************************************/

void FileBuffer::replace_match(MatchSet::iterator pos, const Glib::ustring& substitution)
{
  const Glib::ustring substituted_text
      (Util::substitute_references(substitution.raw(), (*pos)->subject.raw(), (*pos)->captures));

  // Get the start of the match.
  const iterator start ((*pos)->mark->get_iter());

  const int match_length = (*pos)->get_match_length();

  if(match_length > 0)
  {
    // Find end of match.
    iterator stop (start);
    stop.forward_chars(match_length);

    // Replace match with new substituted text.
    insert(erase(start, stop), substituted_text); // triggers on_erase() and on_insert()
  }
  else // empty match
  {
    if(user_action_stack_)
    {
      user_action_stack_->push(UndoActionPtr(
          new FileBufferActionRemoveMatch(*this, start.get_offset(), *pos)));
    }

    // Manually remove match mark and insert the new text.
    delete_mark((*current_match_)->mark); // triggers on_mark_deleted()
    insert(start, substituted_text);      // triggers on_insert()
  }
}

/* Remove the match at position start.  The removal includes the match's
 * Mark object and the tags applied to it.
 */
void FileBuffer::remove_match_at_iter(const FileBuffer::iterator& start)
{
  if(!start.get_marks().empty())
  {
    typedef std::list< Glib::RefPtr<Mark> > MarkList;
    const MarkList marks (start.get_marks());

    for(MarkList::const_iterator pmark = marks.begin(); pmark != marks.end(); ++pmark)
    {
      const MatchDataPtr match_data = MatchData::get_from_mark(*pmark);

      if(!match_data)
        continue; // not a match mark

      if(user_action_stack_)
      {
        user_action_stack_->push(UndoActionPtr(
            new FileBufferActionRemoveMatch(*this, start.get_offset(), match_data)));
      }

      const int match_length = match_data->get_match_length();

      if(match_length > 0)
      {
        const Glib::RefPtr<RegexxerTags>& tagtable = RegexxerTags::instance();

        iterator stop (start);
        stop.forward_chars(match_length);

        remove_tag(tagtable->match, start, stop);

        if(start.begins_tag(tagtable->current))
          remove_tag(tagtable->current, start, stop);
      }

      delete_mark(*pmark); // triggers on_mark_deleted()
    }
  }
}

void FileBuffer::remove_tag_current()
{
  // If we're called just after a removal, then current_match_ already points
  // to the next match but it doesn't have the "current-match" tag set.  So we
  // skip removal of the "current-match" tag since it doesn't exist anymore.

  if(!match_removed_ && current_match_ != match_set_.end())
  {
    const Glib::RefPtr<RegexxerTags>& tagtable = RegexxerTags::instance();

    // Get the start position of the current match.
    const iterator start ((*current_match_)->mark->get_iter());

    const int match_length = (*current_match_)->get_match_length();

    if(match_length > 0)
    {
      // Find the end position of the current match.
      iterator stop (start);
      stop.forward_chars(match_length);

      remove_tag(tagtable->current, start, stop);
    }
    else // empty match
    {
      (*current_match_)->mark->set_visible(false);
    }

    signal_preview_line_changed.queue();
  }
}

void FileBuffer::apply_tag_current()
{
  if(!match_removed_ && current_match_ != match_set_.end())
  {
    const Glib::RefPtr<RegexxerTags>& tagtable = RegexxerTags::instance();

    // Get the start position of the match.
    const iterator start ((*current_match_)->mark->get_iter());

    const int match_length = (*current_match_)->get_match_length();

    if(match_length > 0)
    {
      // Find the end position of the match.
      iterator stop (start);
      stop.forward_chars(match_length);

      apply_tag(tagtable->current, start, stop);
    }
    else // empty match
    {
      (*current_match_)->mark->set_visible(true);
    }

    place_cursor(start);

    signal_preview_line_changed.queue();
  }
}

// static
bool FileBuffer::is_match_start(const iterator& where)
{
  typedef std::list< Glib::RefPtr<Mark> > MarkList;
  const MarkList marks (where.get_marks());

  return (std::find_if(marks.begin(), marks.end(), &MatchData::is_match_mark) != marks.end());
}

// static
void FileBuffer::find_line_bounds(FileBuffer::iterator& line_begin, FileBuffer::iterator& line_end)
{
  line_begin.set_line_index(0);

  if(!line_end.ends_line())
    line_end.forward_to_line_end();
}

void FileBuffer::update_bound_state()
{
  const BoundState old_bound_state = bound_state_;

  get_bound_state(); // recalculate bound_state_

  if(bound_state_ != old_bound_state)
    signal_bound_state_changed(bound_state_); // emit
}

} // namespace Regexxer

