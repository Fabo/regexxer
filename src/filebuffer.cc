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
#include "pcreshell.h"
#include "stringutils.h"

#include <algorithm>
#include <glibmm.h>


namespace
{

class MatchDataMarkEqual // unary predicate
{
  Glib::RefPtr<Gtk::TextMark> mark_;

public:
  explicit MatchDataMarkEqual(const Glib::RefPtr<Gtk::TextMark>& mark)
    : mark_ (mark) {}

  bool operator()(const Regexxer::MatchData& data) const
    { return (mark_ == data.mark); }
};


class RegexxerTags : public Gtk::TextTagTable
{
public:
  typedef Glib::RefPtr<Gtk::TextTag> TextTagPtr;

  // This is a global singleton shared by all FileBuffer instances.
  static const Glib::RefPtr<RegexxerTags>& instance();

  TextTagPtr match;
  TextTagPtr current;

protected:
  RegexxerTags();
  virtual ~RegexxerTags();
};

RegexxerTags::RegexxerTags()
:
  Gtk::TextTagTable(),
  match         (Gtk::TextTag::create("regexxer-match")),
  current       (Gtk::TextTag::create("regexxer-current-match"))
{
  match  ->property_background() = "orange";
  current->property_background() = "yellow";
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

Glib::Quark file_buffer_match_quark()
{
  // Regexxer::FileBuffer uses anonymous Gtk::TextMark objects to remember
  // the position of matches.  This quark is used to identify a match mark,
  // in order to be able to distinguish it from other anonymous marks.

  static Glib::Quark quark ("regexxer-file-buffer-match-quark");
  return quark;
}

int calculate_match_length(const Glib::ustring& subject, const std::pair<int,int>& bounds)
{
  const std::string::const_iterator begin = subject.begin().base();

  const Glib::ustring::const_iterator start (begin + bounds.first);
  const Glib::ustring::const_iterator stop  (begin + bounds.second);

  return std::distance(start, stop);
}

} // anonymous namespace


namespace Regexxer
{

/**** Regexxer::MatchData **************************************************/

MatchData::MatchData(const Glib::RefPtr<Gtk::TextMark>& position,
                     const Glib::ustring& line, const Pcre::Pattern& pattern, int capture_count)
:
  mark    (position),
  subject (line)
{
  captures.reserve(capture_count);

  for(int i = 0; i < capture_count; ++i)
    captures.push_back(pattern.get_substring_bounds(i));
}

MatchData::~MatchData()
{}

int MatchData::get_bytes_length() const
{
  const std::pair<int,int>& whole_match = captures.front();
  return (whole_match.second - whole_match.first);
}


/**** Regexxer::FileBuffer *************************************************/

FileBuffer::FileBuffer()
:
  Gtk::TextBuffer       (RegexxerTags::instance()),
  match_list_           (),
  match_count_          (0),
  current_match_        (match_list_.end()),
  match_removed_        (false),
  bound_state_          (BOUND_FIRST | BOUND_LAST),
  preview_line_changed_ (false)
{}

FileBuffer::~FileBuffer()
{}

// static
Glib::RefPtr<FileBuffer> FileBuffer::create()
{
  return Glib::RefPtr<FileBuffer>(new FileBuffer());
}

/* Apply pattern on all lines in the buffer and return the number of matches.
 * If multiple is false then every line is matched only once, otherwise
 * multiple matches per line will be found (like modifier /g in Perl).
 */
int FileBuffer::find_matches(Pcre::Pattern& pattern, bool multiple)
{
  const Glib::RefPtr<Glib::MainContext> main_context = Glib::MainContext::get_default();
  const Glib::RefPtr<RegexxerTags>& tagtable = RegexxerTags::instance();

  forget_current_match();
  remove_tag(tagtable->match, begin(), end());

  while(!match_list_.empty())
    delete_mark(match_list_.front().mark);

  g_return_val_if_fail(match_count_ == 0, 0);

  for(iterator line = begin(); !line.is_end(); line.forward_line())
  {
    while(main_context->iteration(false)) {}

    iterator line_end (line);

    if(!line_end.ends_line())
      line_end.forward_to_line_end();

    const Glib::ustring subject (get_slice(line, line_end));
    int  offset = 0;
    bool do_match_empty = true;

    do
    {
      const int capture_count = pattern.match(
          subject, offset, (do_match_empty) ? Pcre::MatchOptions(0) : Pcre::NOT_EMPTY);

      if(capture_count <= 0)
      {
        if(!do_match_empty && unsigned(offset) < subject.bytes())
        {
          ++offset;
          do_match_empty = true;
          continue;
        }
        break;
      }

      ++match_count_;

      const std::pair<int,int> bounds (pattern.get_substring_bounds(0));
      const int match_length = calculate_match_length(subject, bounds);

      iterator start (line);
      iterator stop  (line);

      start.set_line_index(bounds.first);
      stop .set_line_index(bounds.second);

      match_list_.push_back(MatchData(
          create_match_mark(start, match_length), subject, pattern, capture_count));

      apply_tag(tagtable->match, start, stop);

      do_match_empty = (match_length > 0);
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
    if(current_match_ != match_list_.end())
      ++current_match_;
    else
      current_match_ = match_list_.begin();
  }

  if(!move_forward)
  {
    if(current_match_ != match_list_.begin())
      --current_match_;
    else
      current_match_ = match_list_.end();
  }

  // See above; we can now safely reset this flag.
  match_removed_ = false;

  apply_tag_current();
  update_bound_state();

  return (current_match_ != match_list_.end()) ? current_match_->mark : Glib::RefPtr<Mark>();
}

/* Remove the highlight tag from the currently selected match and forget
 * about it.  The next call to get_next_match() will start over at the
 * first respectively last match in the buffer.
 */
void FileBuffer::forget_current_match()
{
  remove_tag_current();
  current_match_ = match_list_.end();
  match_removed_ = false;
}

BoundState FileBuffer::get_bound_state()
{
  bound_state_ = BOUND_NONE;

  if(match_list_.empty())
  {
    bound_state_ = BOUND_FIRST | BOUND_LAST;
  }
  else if(current_match_ != match_list_.end())
  {
    if(current_match_ == match_list_.begin())
      bound_state_ |= BOUND_FIRST;

    if(!match_removed_ && current_match_ == --match_list_.end())
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
  if(!match_removed_ && current_match_ != match_list_.end())
  {
    replace_match(current_match_, substitution);
  }
}

void FileBuffer::replace_all_matches(const Glib::ustring& substitution)
{
  while(!match_list_.empty())
    replace_match(match_list_.begin(), substitution);
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

  if(!match_removed_ && current_match_ != match_list_.end())
  {
    // Get the start of the match.
    const iterator start (current_match_->mark->get_iter());

    // Find the end of the match.
    iterator stop (start);
    stop.set_line_index(start.get_line_index() + current_match_->get_bytes_length());

    // Find begin and end of the line containing the match.
    iterator line_begin (start);
    iterator line_end   (stop);
    find_line_bounds(line_begin, line_end);

    const std::string& subject = current_match_->subject.raw();

    // Construct the preview line: [line_begin,start) + substitution + [stop,line_end)
    preview  = get_text(line_begin, start);
    preview += Util::substitute_references(substitution.raw(), subject, current_match_->captures);
    position = preview.length();
    preview += get_text(stop, line_end);
  }

  return position;
}

/**** Regexxer::FileBuffer -- protected ************************************/

void FileBuffer::on_insert(const FileBuffer::iterator& pos, const Glib::ustring& text, int bytes)
{
  if(!text.empty())
  {
    if(!match_removed_ && current_match_ != match_list_.end())
    {
      // Test whether pos is within the current match and push
      // signal_preview_line_changed() on the queue if true.

      iterator lbegin (current_match_->mark->get_iter());
      iterator lend   (lbegin);
      find_line_bounds(lbegin, lend);

      if(pos.in_range(lbegin, lend))
        trigger_preview_line_changed();
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

  Gtk::TextBuffer::on_insert(pos, text, bytes);
}

void FileBuffer::on_erase(const FileBuffer::iterator& rbegin, const FileBuffer::iterator& rend)
{
  if(!match_removed_ && current_match_ != match_list_.end())
  {
    // Test whether [rbegin,rend) overlaps with the current match
    // and push signal_preview_line_changed() on the queue if true.

    iterator lbegin (current_match_->mark->get_iter());
    iterator lend   (lbegin);
    find_line_bounds(lbegin, lend);

    if(lbegin.in_range(rbegin, rend) || rbegin.in_range(lbegin, lend))
      trigger_preview_line_changed();
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
        match_length = std::max(match_length, get_match_length(*pmark));
    }
    while(match_length < 0 && !pos.starts_line() && !pos.toggles_tag(tagtable->match));

    if(match_length > backward_chars)
      remove_match_at_iter(pos);
  }

  for(iterator pos = rbegin; pos != rend; ++pos)
    remove_match_at_iter(pos);

  Gtk::TextBuffer::on_erase(rbegin, rend);
}

void FileBuffer::on_mark_deleted(const Glib::RefPtr<TextBuffer::Mark>& mark)
{
  Gtk::TextBuffer::on_mark_deleted(mark);

  // If a mark has been deleted we check whether it was one of our match
  // marks and if so, remove it from the list.  Deletion of the current
  // match has to be special cased, but's also a bit faster this way since
  // it's the most common situation and we don't need to traverse the list.

  if(current_match_ != match_list_.end() && current_match_->mark == mark)
  {
    current_match_ = match_list_.erase(current_match_);
    match_removed_ = true;

    signal_match_count_changed(--match_count_); // emit
    update_bound_state();
  }
  else if(is_match_mark(mark))
  {
    const std::list<MatchData>::iterator pos =
        std::find_if(match_list_.begin(), match_list_.end(), MatchDataMarkEqual(mark));

    if(pos != match_list_.end())
    {
      match_list_.erase(pos);

      signal_match_count_changed(--match_count_); // emit
      update_bound_state();
    }
  }
}

/**** Regexxer::FileBuffer -- private **************************************/

void FileBuffer::replace_match(std::list<MatchData>::iterator pos, const Glib::ustring& substitution)
{
  const Glib::ustring substituted_text
      (Util::substitute_references(substitution.raw(), pos->subject.raw(), pos->captures));

  // Get the start of the match.
  const iterator start (pos->mark->get_iter());

  const int bytes_length = current_match_->get_bytes_length();

  if(bytes_length > 0)
  {
    // Find end of match.
    iterator stop (start);
    stop.set_line_index(start.get_line_index() + bytes_length);

    // Replace match with new substituted text.
    insert(erase(start, stop), substituted_text); // triggers on_erase() and on_insert()
  }
  else // empty match
  {
    // Manually remove match mark and insert the new text.
    delete_mark(current_match_->mark); // triggers on_mark_deleted()
    insert(start, substituted_text);   // triggers on_insert()
  }
}

Glib::RefPtr<FileBuffer::Mark>
FileBuffer::create_match_mark(const FileBuffer::iterator& where, int length)
{
  const Glib::RefPtr<Mark> mark (create_mark(where, false)); // right gravity

  // Mark this anonymous mark as match mark ;)
  mark->set_data(file_buffer_match_quark(), GINT_TO_POINTER(length + 1));

  return mark;
}

// static
bool FileBuffer::is_match_mark(const Glib::RefPtr<FileBuffer::Mark>& mark)
{
  return (mark->get_data(file_buffer_match_quark()) != 0);
}

// static
int FileBuffer::get_match_length(const Glib::RefPtr<FileBuffer::Mark>& mark)
{
  if(void *const qdata = mark->get_data(file_buffer_match_quark()))
    return GPOINTER_TO_INT(qdata) - 1;

  return -1;
}

// static
bool FileBuffer::is_match_start(const iterator& where)
{
  typedef std::list< Glib::RefPtr<Mark> > MarkList;
  const MarkList marks (where.get_marks());

  return (std::find_if(marks.begin(), marks.end(), &FileBuffer::is_match_mark) != marks.end());
}

void FileBuffer::remove_tag_current()
{
  // If we're called just after a removal, then current_match_ already points
  // to the next match but it doesn't have the "current-match" tag set.  So we
  // skip removal of the "current-match" tag since it doesn't exist anymore.

  if(!match_removed_ && current_match_ != match_list_.end())
  {
    const Glib::RefPtr<RegexxerTags>& tagtable = RegexxerTags::instance();

    // Get the start position of the current match.
    const iterator start (current_match_->mark->get_iter());

    const int bytes_length = current_match_->get_bytes_length();

    if(bytes_length > 0)
    {
      // Find the end position of the current match.
      iterator stop (start);
      stop.set_line_index(start.get_line_index() + bytes_length);

      remove_tag(tagtable->current, start, stop);
    }
    else // empty match
    {
      current_match_->mark->set_visible(false);
    }

    trigger_preview_line_changed();
  }
}

void FileBuffer::apply_tag_current()
{
  if(!match_removed_ && current_match_ != match_list_.end())
  {
    const Glib::RefPtr<RegexxerTags>& tagtable = RegexxerTags::instance();

    // Get the start position of the match.
    const iterator start (current_match_->mark->get_iter());

    const int bytes_length = current_match_->get_bytes_length();

    if(bytes_length > 0)
    {
      // Find the end position of the match.
      iterator stop (start);
      stop.set_line_index(start.get_line_index() + bytes_length);

      apply_tag(tagtable->current, start, stop);
    }
    else // empty match
    {
      current_match_->mark->set_visible(true);
    }

    place_cursor(start);

    trigger_preview_line_changed();
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
      const int match_length = get_match_length(*pmark);

      if(match_length < 0)
        continue; // not a match mark

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

void FileBuffer::trigger_preview_line_changed()
{
  // Collect subsequent emission requests and emit only once,
  // as soon as the GLib main loop is idle again.

  if(!preview_line_changed_)
  {
    preview_line_changed_ = true;

    Glib::signal_idle().connect(
        SigC::slot(*this, &FileBuffer::preview_line_changed_idle_callback),
        Glib::PRIORITY_HIGH_IDLE);
  }
}

bool FileBuffer::preview_line_changed_idle_callback()
{
  signal_preview_line_changed(); // emit

  // Tell trigger_preview_line_changed() that we've done the update.
  preview_line_changed_ = false;

  return false; // disconnect callback
}

} // namespace Regexxer

