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

#ifndef REGEXXER_FILEBUFFER_H_INCLUDED
#define REGEXXER_FILEBUFFER_H_INCLUDED

#include "fileshared.h"
#include "signalutils.h"
#include "undostack.h"

#include <gtkmm/textbuffer.h>
#include <set>


namespace Regexxer
{

class FileBuffer : public Gtk::TextBuffer
{
public:
  static Glib::RefPtr<FileBuffer> create();
  static Glib::RefPtr<FileBuffer> create_with_error_message(
      const Glib::RefPtr<Gdk::Pixbuf>& pixbuf, const Glib::ustring& message);

  static void pango_context_changed(const Glib::RefPtr<Pango::Context>& context);

  static void set_match_color(const Gdk::Color& color);
  static void set_current_color(const Gdk::Color& color);
  static Gdk::Color get_match_color();
  static Gdk::Color get_current_color();

  virtual ~FileBuffer();

  bool is_freeable() const;
  bool in_user_action() const;

  int find_matches(Pcre::Pattern& pattern, bool multiple);

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

  sigc::signal<void, int>           signal_match_count_changed;
  sigc::signal<void, BoundState>    signal_bound_state_changed;
  Util::QueuedSignal                signal_preview_line_changed;
  sigc::signal<bool>               signal_pulse;
  sigc::signal<void, UndoActionPtr> signal_undo_stack_push;

protected:
  FileBuffer();

  virtual void on_insert(const iterator& pos, const Glib::ustring& text, int bytes);
  virtual void on_erase(const iterator& rbegin, const iterator& rend);
  virtual void on_mark_deleted(const Glib::RefPtr<TextBuffer::Mark>& mark);
  virtual void on_modified_changed();
  virtual void on_begin_user_action();
  virtual void on_end_user_action();

private:
  class ScopedLock;
  class ScopedUserAction;

  typedef std::set<MatchDataPtr,MatchDataLess> MatchSet;

  MatchSet            match_set_;
  int                 match_count_;
  int                 original_match_count_;
  MatchSet::iterator  current_match_;
  bool                match_removed_;
  BoundState          bound_state_;
  bool                locked_;
  UndoStackPtr        user_action_stack_;
  unsigned long       stamp_modified_;
  unsigned long       stamp_saved_;

  void replace_match(MatchSet::iterator pos, const Glib::ustring& substitution);
  void remove_match_at_iter(const iterator& start);

  void remove_tag_current();
  void apply_tag_current();

  static bool is_match_start(const iterator& where);
  static void find_line_bounds(iterator& line_begin, iterator& line_end);

  void update_bound_state();

  // Work-around for silly, stupid, and annoying gcc 2.95.x.
  friend class FileBuffer::ScopedLock;
};

} // namespace Regexxer

#endif /* REGEXXER_FILEBUFFER_H_INCLUDED */

