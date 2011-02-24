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

#ifndef REGEXXER_FILEBUFFER_H_INCLUDED
#define REGEXXER_FILEBUFFER_H_INCLUDED

#include "fileshared.h"
#include "signalutils.h"
#include "undostack.h"

#include <gtksourceviewmm/sourcebuffer.h>
#include <set>
#include <stack>


namespace Regexxer
{

class FileBufferActionRemoveMatch;


class FileBuffer : public Gsv::SourceBuffer
{
public:
  static Glib::RefPtr<FileBuffer> create();
  static Glib::RefPtr<FileBuffer> create_with_error_message(
      const Glib::RefPtr<Gdk::Pixbuf>& pixbuf, const Glib::ustring& message);

  static void pango_context_changed(const Glib::RefPtr<Pango::Context>& context);

  virtual ~FileBuffer();

  bool is_freeable() const;
  bool in_user_action() const;

  int find_matches(const Glib::RefPtr<Glib::Regex>& pattern, bool multiple,
                   const sigc::slot<void, int, const Glib::ustring&>& feedback);

  int get_match_count() const;
  int get_match_index() const;
  int get_original_match_count() const;

  Glib::RefPtr<Mark> get_next_match(bool move_forward);
  void forget_current_match();

  BoundState get_bound_state();

  void replace_current_match(const Glib::ustring& substitution);
  void replace_all_matches(const Glib::ustring& substitution);

  int get_line_preview(const Glib::ustring& substitution, Glib::ustring& preview);

  // Special API for the FileBufferAction classes.
  void increment_stamp();
  void decrement_stamp();
  void undo_remove_match(const MatchDataPtr& match, int offset);
  void undo_add_weak(FileBufferActionRemoveMatch* ptr);
  void undo_remove_weak(FileBufferActionRemoveMatch* ptr);

  sigc::signal<void>                signal_match_count_changed;
  sigc::signal<void>                signal_bound_state_changed;
  Util::QueuedSignal                signal_preview_line_changed;
  sigc::signal<bool>                signal_pulse;
  sigc::signal<void, UndoActionPtr> signal_undo_stack_push;

protected:
  FileBuffer();

  virtual void on_insert(const iterator& pos, const Glib::ustring& text, int bytes);
  virtual void on_erase(const iterator& rbegin, const iterator& rend);
  virtual void on_mark_deleted(const Glib::RefPtr<Mark>& mark);
  virtual void on_apply_tag(const Glib::RefPtr<Tag>& tag,
                            const iterator& range_begin, const iterator& range_end);
  virtual void on_modified_changed();
  virtual void on_begin_user_action();
  virtual void on_end_user_action();

private:
  class ScopedLock;
  class ScopedUserAction;

  typedef std::set<MatchDataPtr, MatchDataLess>     MatchSet;
  typedef std::stack<FileBufferActionRemoveMatch*>  WeakUndoStack;

  MatchSet            match_set_;
  MatchSet::iterator  current_match_;
  UndoStackPtr        user_action_stack_;
  WeakUndoStack       weak_undo_stack_;
  int                 match_count_;
  int                 original_match_count_;
  unsigned long       stamp_modified_;
  unsigned long       stamp_saved_;
  BoundState          cached_bound_state_;
  bool                match_removed_;
  bool                locked_;

  void replace_match(MatchSet::const_iterator pos, const Glib::ustring& substitution);
  void remove_match_at_iter(const iterator& start);

  void remove_tag_current();
  void apply_tag_current();

  static bool is_match_start(const iterator& where);
  static void find_line_bounds(iterator& line_begin, iterator& line_end);

  void update_bound_state();
  void notify_weak_undos();
};

} // namespace Regexxer

#endif /* REGEXXER_FILEBUFFER_H_INCLUDED */
