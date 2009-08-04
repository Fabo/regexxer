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

#include "filebuffer.h"
#include "filebufferundo.h"
#include "globalstrings.h"
#include "miscutils.h"
#include "pcreshell.h"
#include "stringutils.h"
#include "translation.h"

#include <glib.h>
#include <gconfmm/client.h>
#include <algorithm>
#include <list>

#include <config.h>

namespace
{

enum { PULSE_INTERVAL = 128 };

class RegexxerTags : public Gtk::TextTagTable
{
public:
  typedef Glib::RefPtr<Gtk::TextTag> TextTagPtr;

  // This is a global singleton shared by all FileBuffer instances.
  static Glib::RefPtr<RegexxerTags> instance();

  TextTagPtr error_message;
  TextTagPtr error_title;
  TextTagPtr match;
  TextTagPtr current;

protected:
  RegexxerTags();
  virtual ~RegexxerTags();

private:
  void on_conf_value_changed(const Glib::ustring& key, const Gnome::Conf::Value& value);
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
  error_message->property_pixels_above_lines() = 16;

  error_title->property_scale() = Pango::SCALE_X_LARGE;

  Gnome::Conf::Client::get_default_client()->signal_value_changed()
      .connect(sigc::mem_fun(*this, &RegexxerTags::on_conf_value_changed));

  add(error_message);
  add(error_title);
  add(match);
  add(current);
}

RegexxerTags::~RegexxerTags()
{}

void RegexxerTags::on_conf_value_changed(const Glib::ustring& key, const Gnome::Conf::Value& value)
{
  using namespace Regexxer;

  if (value.get_type() == Gnome::Conf::VALUE_STRING)
  {
    if (key.raw() == conf_key_match_color)
      match->property_background() = value.get_string();

    else if (key.raw() == conf_key_current_match_color)
      current->property_background() = value.get_string();
  }
}

// static
Glib::RefPtr<RegexxerTags> RegexxerTags::instance()
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
  gtksourceview::SourceBuffer(Glib::RefPtr<Gtk::TextTagTable>(RegexxerTags::instance())),
  match_set_            (),
  current_match_        (match_set_.end()),
  user_action_stack_    (),
  weak_undo_stack_      (),
  match_count_          (0),
  original_match_count_ (0),
  stamp_modified_       (0),
  stamp_saved_          (0),
  cached_bound_state_   (BOUND_FIRST | BOUND_LAST),
  match_removed_        (false),
  locked_               (false)
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
  const Glib::RefPtr<RegexxerTags> tagtable = RegexxerTags::instance();

  iterator pend = buffer->insert_pixbuf(buffer->end(), pixbuf);

  Glib::ustring title = "\302\240"; // U+00A0 NO-BREAK SPACE

  title += _("Can\342\200\231t read file:");
  title += '\n';

  pend = buffer->insert_with_tag(pend, title, tagtable->error_title);
  pend = buffer->insert(pend, message);

  if (!message.empty() && !Glib::Unicode::ispunct(*message.rbegin()))
    pend = buffer->insert(pend, ".");

  buffer->apply_tag(tagtable->error_message, buffer->begin(), pend);
  buffer->set_modified(false);

  return buffer;
}

// static
void FileBuffer::pango_context_changed(const Glib::RefPtr<Pango::Context>& context)
{
  const Pango::FontDescription font_description = context->get_font_description();

  // This magic code calculates the height to raise the error message title,
  // so that it's displayed approximately in line with the error icon. By
  // default the text would appear at the bottom, and since the icon is
  // about 48 pixels tall this looks incredibly ugly.

  int font_size = font_description.get_size();

  if (font_size <= 0) // urgh, fall back to some reasonable value
    font_size = 10 * Pango::SCALE;

  int icon_width = 0, icon_height = 0;
  Gtk::IconSize::lookup(Gtk::ICON_SIZE_DIALOG, icon_width, icon_height);

  g_return_if_fail(icon_height > 0); // the lookup should never fail for builtin icon sizes

  const int title_size  = int(Pango::SCALE_X_LARGE * font_size);
  const int rise_height = (icon_height * Pango::SCALE - title_size) / 2;

  const Glib::RefPtr<RegexxerTags> tagtable = RegexxerTags::instance();

  tagtable->error_message->property_font_desc() = font_description;
  tagtable->error_title  ->property_rise()      = rise_height;
}

bool FileBuffer::is_freeable() const
{
  return (!locked_ && match_count_ == 0 && stamp_modified_ == 0 && stamp_saved_ == 0);
}

bool FileBuffer::in_user_action() const
{
  return (user_action_stack_ != 0);
}

/*
 * Apply pattern on all lines in the buffer and return the number of matches.
 * If multiple is false then every line is matched only once, otherwise
 * multiple matches per line will be found (like modifier /g in Perl).
 */
int FileBuffer::find_matches(Pcre::Pattern& pattern, bool multiple,
                             const sigc::slot<void, int, const Glib::ustring&>& feedback)
{
  ScopedLock lock (*this);

  const Glib::RefPtr<RegexxerTags> tagtable = RegexxerTags::instance();

  notify_weak_undos();
  forget_current_match();
  remove_tag(tagtable->match, begin(), end());

  while (!match_set_.empty())
    delete_mark((*match_set_.begin())->mark); // triggers on_mark_deleted()

  g_return_val_if_fail(match_count_ == 0, 0);
  original_match_count_ = 0;

  unsigned int iteration = 0;

  for (iterator line = begin(); !line.is_end(); line.forward_line())
  {
    iterator line_end = line;

    if (!line_end.ends_line())
      line_end.forward_to_line_end();

    const Glib::ustring subject = get_slice(line, line_end);
    int  offset = 0;
    bool last_was_empty = false;

    do
    {
      if ((++iteration % PULSE_INTERVAL) == 0 && signal_pulse()) // emit
      {
        signal_match_count_changed(); // emit
        return match_count_;
      }

      const int capture_count =
        pattern.match(subject, offset, (last_was_empty) ? Pcre::ANCHORED | Pcre::NOT_EMPTY
                                                        : Pcre::MatchOptions(0));
      if (capture_count <= 0)
      {
        if (last_was_empty && unsigned(offset) < subject.bytes())
        {
          const std::string::const_iterator pbegin = subject.begin().base();
          Glib::ustring::const_iterator     poffset (pbegin + offset);

          offset = (++poffset).base() - pbegin; // forward one UTF-8 character
          last_was_empty = false;
          continue;
        }
        break;
      }

      ++match_count_;
      ++original_match_count_;

      const std::pair<int, int> bounds = pattern.get_substring_bounds(0);

      iterator start = line;
      iterator stop  = line;

      start.set_line_index(bounds.first);
      stop .set_line_index(bounds.second);

      const MatchDataPtr match (new MatchData(
          original_match_count_, subject, pattern, capture_count));

      match_set_.insert(match_set_.end(), match);
      match->install_mark(start);

      apply_tag(tagtable->match, start, stop);

      if (offset == 0 && feedback)
        feedback(line.get_line(), subject);

      last_was_empty = (bounds.first == bounds.second);
      offset = bounds.second;
    }
    while (multiple);
  }

  signal_match_count_changed(); // emit
  return match_count_;
}

int FileBuffer::get_match_count() const
{
  return match_count_;
}

int FileBuffer::get_match_index() const
{
  return (!match_removed_ && current_match_ != match_set_.end()) ? (*current_match_)->index : 0;
}

int FileBuffer::get_original_match_count() const
{
  return original_match_count_;
}

/*
 * Move to the next match in the buffer.  If there is a next match
 * its position will be returned, otherwise 0.
 */
Glib::RefPtr<FileBuffer::Mark> FileBuffer::get_next_match(bool move_forward)
{
  remove_tag_current();

  // If we're called just after a removal, then current_match_ already points
  // to the next match but it doesn't have the "current-match" tag set.  So we
  // prevent moving forward since this would cause one match to be skipped.
  // Moving backward is OK though.

  if (move_forward && !match_removed_)
  {
    if (current_match_ != match_set_.end())
      ++current_match_;
    else
      current_match_ = match_set_.begin();
  }

  if (!move_forward)
  {
    if (current_match_ != match_set_.begin())
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

/*
 * Remove the highlight tag from the currently selected match and forget
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
  BoundState bound = BOUND_NONE;

  if (match_set_.empty())
  {
    bound = BOUND_FIRST | BOUND_LAST;
  }
  else if (current_match_ != match_set_.end())
  {
    if (current_match_ == match_set_.begin())
      bound |= BOUND_FIRST;

    if (!match_removed_ && current_match_ == --match_set_.end())
      bound |= BOUND_LAST;
  }

  cached_bound_state_ = bound;
  return bound;
}

/*
 * Replace the currently selected match with substitution.  References
 * to captured substrings in substitution will be interpolated.  This
 * method indirectly triggers emission of signal_match_count_changed()
 * and signal_preview_line_changed().
 */
void FileBuffer::replace_current_match(const Glib::ustring& substitution)
{
  if (!match_removed_ && current_match_ != match_set_.end())
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

  while (!match_set_.empty())
  {
    replace_match(match_set_.begin(), substitution);

    if ((++iteration % PULSE_INTERVAL) == 0 && signal_pulse()) // emit
      break;
  }
}

/*
 * Build a preview of what replace_current_match() would do to the current
 * line if it were called with substitution as argument.  References to
 * captured substrings in substitution will be interpolated.  The result is
 * written to the output argument preview.  The return value is a character
 * offset into preview pointing to the end of the replaced text.
 */
int FileBuffer::get_line_preview(const Glib::ustring& substitution, Glib::ustring& preview)
{
  int position = -1;
  Glib::ustring result;

  if (!match_removed_ && current_match_ != match_set_.end())
  {
    const MatchDataPtr match = *current_match_;

    // Get the start of the match.
    const iterator start = match->mark->get_iter();

    // Find the end of the match.
    iterator stop = start;
    stop.forward_chars(match->length);

    // Find begin and end of the line containing the match.
    iterator line_begin = start;
    iterator line_end   = stop;
    find_line_bounds(line_begin, line_end);

    // Construct the preview line: [line_begin,start) + substitution + [stop,line_end)
    result   = get_text(line_begin, start);
    result  += Util::substitute_references(substitution, match->subject, match->captures);
    position = result.length();
    result  += get_text(stop, line_end);
  }

  swap(result, preview);
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
  const std::pair<MatchSet::iterator, bool> pos = match_set_.insert(match);
  g_return_if_fail(pos.second);

  const iterator start = get_iter_at_offset(offset);
  match->install_mark(start);

  if (match->length > 0)
  {
    const Glib::RefPtr<RegexxerTags> tagtable = RegexxerTags::instance();

    iterator stop = start;
    stop.forward_chars(match->length);

    apply_tag(tagtable->match, start, stop);
  }

  if (match_removed_ && Util::next(pos.first) == current_match_)
  {
    current_match_ = pos.first;
    match_removed_ = false;

    apply_tag_current();
  }

  signal_preview_line_changed.queue();

  ++match_count_;
  signal_match_count_changed(); // emit
  update_bound_state();
}

/*
 * Unfortunately it turned out to be necessary to keep track of UndoActions
 * that reference MatchData objects, since find_matches() needs some way of
 * telling the UndoAction objects to drop their references.  Omitting this
 * causes clashes between new matches and the obsolete ones which are still
 * assumed to be valid by the undo actions.  (In short: if it doesn't crash,
 * it's still going to leak out memory like the Titanic leaked in water.)
 */
void FileBuffer::undo_add_weak(FileBufferActionRemoveMatch* ptr)
{
  weak_undo_stack_.push(ptr);
}

void FileBuffer::undo_remove_weak(FileBufferActionRemoveMatch* ptr)
{
  // Thanks to the strict LIFO semantics of UndoStack it's possible
  // to implement the weak references as stack too, thus reducing the
  // book-keeping overhead to a bare minimum.
  g_return_if_fail(weak_undo_stack_.top() == ptr);
  weak_undo_stack_.pop();
}

/**** Regexxer::FileBuffer -- protected ************************************/

void FileBuffer::on_insert(const FileBuffer::iterator& pos, const Glib::ustring& text, int bytes)
{
  if (!text.empty())
  {
    if (!match_removed_ && current_match_ != match_set_.end())
    {
      // Test whether pos is within the current match and push
      // signal_preview_line_changed() on the queue if true.

      iterator lbegin = (*current_match_)->mark->get_iter();
      iterator lend   = lbegin;
      find_line_bounds(lbegin, lend);

      if (pos.in_range(lbegin, lend))
        signal_preview_line_changed.queue();
    }

    const Glib::RefPtr<RegexxerTags> tagtable = RegexxerTags::instance();

    // If pos is within a match then remove this match.
    if (pos.has_tag(tagtable->match) && !is_match_start(pos))
    {
      iterator start = pos;
      start.backward_to_tag_toggle(tagtable->match);
      remove_match_at_iter(start);
    }
  }

  if (user_action_stack_)
  {
    user_action_stack_->push(UndoActionPtr(
        new FileBufferActionInsert(*this, pos.get_offset(), text)));
  }

  Gtk::TextBuffer::on_insert(pos, text, bytes);
}

void FileBuffer::on_erase(const FileBuffer::iterator& rbegin, const FileBuffer::iterator& rend)
{
  if (!match_removed_ && current_match_ != match_set_.end())
  {
    // Test whether [rbegin,rend) overlaps with the current match
    // and push signal_preview_line_changed() on the queue if true.

    iterator lbegin = (*current_match_)->mark->get_iter();
    iterator lend   = lbegin;
    find_line_bounds(lbegin, lend);

    if (lbegin.in_range(rbegin, rend) || rbegin.in_range(lbegin, lend))
      signal_preview_line_changed.queue();
  }

  const Glib::RefPtr<const Gtk::TextTag> tag_match = RegexxerTags::instance()->match;

  if (!rbegin.starts_line() && rbegin.has_tag(tag_match))
  {
    g_return_if_fail(!rbegin.ends_tag(tag_match)); // just to be sure...

    int backward_chars = 0;
    int match_length = -1;
    iterator pos = rbegin;

    do
    {
      ++backward_chars;
      --pos;

      typedef std::list< Glib::RefPtr<Mark> > MarkList;
      const MarkList marks (const_cast<iterator&>(pos).get_marks());  // XXX

      for (MarkList::const_iterator pmark = marks.begin(); pmark != marks.end(); ++pmark)
      {
        if (const MatchDataPtr match_data = MatchData::get_from_mark(*pmark))
          match_length = std::max(match_length, match_data->length);
      }
    }
    while (match_length < 0 && !pos.starts_line() && !pos.toggles_tag(tag_match));

    if (match_length > backward_chars)
      remove_match_at_iter(pos);
  }

  for (iterator pos = rbegin; pos != rend; ++pos)
    remove_match_at_iter(pos);

  if (user_action_stack_)
  {
    user_action_stack_->push(UndoActionPtr(
        new FileBufferActionErase(*this, rbegin.get_offset(), get_slice(rbegin, rend))));
  }

  Gtk::TextBuffer::on_erase(rbegin, rend);
}

void FileBuffer::on_mark_deleted(const Glib::RefPtr<FileBuffer::Mark>& mark)
{
  Gtk::TextBuffer::on_mark_deleted(mark);

  // If a mark has been deleted we check whether it was one of our match
  // marks and if so, remove it from the list.  Deletion of the current
  // match has to be special cased, but's also a bit faster this way since
  // it's the most common situation and we don't need to traverse the list.

  if (current_match_ != match_set_.end() && (*current_match_)->mark == mark)
  {
    const MatchSet::iterator pos = current_match_++;

    Glib::RefPtr<FileBuffer::Mark>().swap((*pos)->mark);
    match_set_.erase(pos);
    match_removed_ = true;

    --match_count_;
    signal_match_count_changed(); // emit
    update_bound_state();
  }
  else if (const MatchDataPtr match_data = MatchData::get_from_mark(mark))
  {
    const MatchSet::iterator pos = match_set_.find(match_data);

    if (pos != match_set_.end())
    {
      Glib::RefPtr<FileBuffer::Mark>().swap((*pos)->mark);
      match_set_.erase(pos);

      --match_count_;
      signal_match_count_changed(); // emit
      update_bound_state();
    }
  }
}

void FileBuffer::on_apply_tag(const Glib::RefPtr<FileBuffer::Tag>& tag,
                              const FileBuffer::iterator& range_begin,
                              const FileBuffer::iterator& range_end)
{
  // Ignore tags when inserting text from the clipboard, to avoid confusion
  // caused by highlighting text as match which is not recorded as such
  // internally.  Simply checking if a user action is currently in progress
  // does the trick.
  if (!in_user_action())
    Gtk::TextBuffer::on_apply_tag(tag, range_begin, range_end);
}

void FileBuffer::on_modified_changed()
{
  if (!get_modified())
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

  UndoStackPtr undo_action;
  swap(undo_action, user_action_stack_);

  if (!undo_action->empty())
    signal_undo_stack_push(undo_action); // emit
}

/**** Regexxer::FileBuffer -- private **************************************/

void FileBuffer::replace_match(MatchSet::const_iterator pos, const Glib::ustring& substitution)
{
  const MatchDataPtr match = *pos;

  const Glib::ustring substituted_text =
      Util::substitute_references(substitution, match->subject, match->captures);

  // Get the start of the match.
  const iterator start = match->mark->get_iter();

  if (match->length > 0)
  {
    // Find end of match.
    iterator stop = start;
    stop.forward_chars(match->length);

    // Replace match with new substituted text.
    insert(erase(start, stop), substituted_text); // triggers on_erase() and on_insert()
  }
  else // empty match
  {
    if (user_action_stack_)
    {
      user_action_stack_->push(UndoActionPtr(
          new FileBufferActionRemoveMatch(*this, start.get_offset(), match)));
    }

    // Manually remove match mark and insert the new text.
    delete_mark(match->mark); // triggers on_mark_deleted()

    if (!substituted_text.empty())
      insert(start, substituted_text); // triggers on_insert()
    else
      // Do a dummy insert to avoid special case of empty-by-empty replace.
      on_insert(start, substituted_text, 0);
  }
}

/*
 * Remove the match at position start.  The removal includes the match's
 * Mark object and the tags applied to it.
 */
void FileBuffer::remove_match_at_iter(const FileBuffer::iterator& start)
{
  typedef std::list< Glib::RefPtr<Mark> > MarkList;
  const MarkList marks (const_cast<iterator&>(start).get_marks()); // XXX

  for (MarkList::const_iterator pmark = marks.begin(); pmark != marks.end(); ++pmark)
  {
    const MatchDataPtr match = MatchData::get_from_mark(*pmark);

    if (!match)
      continue; // not a match mark

    if (user_action_stack_)
    {
      user_action_stack_->push(UndoActionPtr(
          new FileBufferActionRemoveMatch(*this, start.get_offset(), match)));
    }

    if (match->length > 0)
    {
      const Glib::RefPtr<RegexxerTags> tagtable = RegexxerTags::instance();

      iterator stop = start;
      stop.forward_chars(match->length);

      remove_tag(tagtable->match, start, stop);

      if (start.begins_tag(tagtable->current))
        remove_tag(tagtable->current, start, stop);
    }

    delete_mark(*pmark); // triggers on_mark_deleted()
  }
}

void FileBuffer::remove_tag_current()
{
  // If we're called just after a removal, then current_match_ already points
  // to the next match but it doesn't have the "current-match" tag set.  So we
  // skip removal of the "current-match" tag since it doesn't exist anymore.

  if (!match_removed_ && current_match_ != match_set_.end())
  {
    const Glib::RefPtr<RegexxerTags> tagtable = RegexxerTags::instance();

    // Get the start position of the current match.
    const iterator start   = (*current_match_)->mark->get_iter();
    const int match_length = (*current_match_)->length;

    if (match_length > 0)
    {
      // Find the end position of the current match.
      iterator stop = start;
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
  if (!match_removed_ && current_match_ != match_set_.end())
  {
    const Glib::RefPtr<RegexxerTags> tagtable = RegexxerTags::instance();

    // Get the start position of the match.
    const iterator start   = (*current_match_)->mark->get_iter();
    const int match_length = (*current_match_)->length;

    if (match_length > 0)
    {
      // Find the end position of the match.
      iterator stop = start;
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
  const MarkList marks (const_cast<iterator&>(where).get_marks()); // XXX

  return (std::find_if(marks.begin(), marks.end(), &MatchData::is_match_mark) != marks.end());
}

// static
void FileBuffer::find_line_bounds(FileBuffer::iterator& line_begin, FileBuffer::iterator& line_end)
{
  line_begin.set_line_index(0);

  if (!line_end.ends_line())
    line_end.forward_to_line_end();
}

void FileBuffer::update_bound_state()
{
  const BoundState old_bound_state = cached_bound_state_;

  if (get_bound_state() != old_bound_state)
    signal_bound_state_changed(); // emit
}

void FileBuffer::notify_weak_undos()
{
  while (!weak_undo_stack_.empty())
  {
    FileBufferActionRemoveMatch *const ptr = weak_undo_stack_.top();
    weak_undo_stack_.pop();
    ptr->weak_notify();
  }
}

} // namespace Regexxer
