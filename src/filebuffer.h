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

#include <list>
#include <utility>
#include <vector>
#include <gtkmm/textbuffer.h>


namespace Pcre { class Pattern; }

namespace Regexxer
{

enum BoundState
{
  BOUND_NONE  = 0,
  BOUND_FIRST = 1 << 0,
  BOUND_LAST  = 1 << 1,
  BOUND_MASK  = BOUND_FIRST | BOUND_LAST
};

inline BoundState operator|(BoundState lhs, BoundState rhs)
  { return static_cast<BoundState>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs)); }

inline BoundState operator&(BoundState lhs, BoundState rhs)
  { return static_cast<BoundState>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs)); }

inline BoundState operator~(BoundState flags)
  { return static_cast<BoundState>(~static_cast<unsigned>(flags)); }


// This struct holds all the information that's necessary to locate a
// match's position in the buffer and to substitute captured substrings
// into a replacement string.  The latter is achived by storing the whole
// subject string (i.e. the line in the buffer) together with a table of
// indices into it.  This arrangement should consume less memory than the
// alternative of storing a vector of captured substrings since we want
// to support $&, $`, $' too.
//
struct MatchData
{
  MatchData(const Glib::RefPtr<Gtk::TextMark>& position,
            const Glib::ustring& line, const Pcre::Pattern& pattern, int capture_count);
  ~MatchData();

  Glib::RefPtr<Gtk::TextMark>       mark;
  Glib::ustring                     subject;
  std::vector< std::pair<int,int> > captures;
};


class FileBuffer : public Gtk::TextBuffer
{
public:
  static Glib::RefPtr<FileBuffer> create();
  virtual ~FileBuffer();

  int find_matches(Pcre::Pattern& pattern, bool multiple);

  BoundState get_bound_state();
  Glib::RefPtr<Mark> get_next_match(bool move_forward);
  void forget_current_match();
  void replace_current_match(const Glib::ustring& substitution);

  int get_line_preview(const Glib::ustring& substitution, Glib::ustring& preview);

  SigC::Signal1<void,int>         signal_match_count_changed;
  SigC::Signal1<void,BoundState>  signal_bound_state_changed;
  SigC::Signal0<void>             signal_preview_line_changed;

protected:
  FileBuffer();

  virtual void on_insert(const iterator& pos, const Glib::ustring& text, int bytes);
  virtual void on_erase(const iterator& rbegin, const iterator& rend);
  virtual void on_mark_deleted(const Glib::RefPtr<TextBuffer::Mark>& mark);

private:
  std::list<MatchData>            match_list_;
  int                             match_count_;
  std::list<MatchData>::iterator  current_match_;
  bool                            match_removed_;
  BoundState                      bound_state_;
  bool                            preview_line_changed_;

  Glib::RefPtr<Mark> create_match_mark(const iterator& where);
  bool is_match_mark(const Glib::RefPtr<Mark>& mark);

  void remove_tag_current();
  void apply_tag_current();
  void remove_match_at_iter(const iterator& where);

  static void find_line_bounds(iterator& line_begin, iterator& line_end);

  void update_bound_state();
  void trigger_preview_line_changed();
  bool preview_line_changed_idle_callback();
};

} // namespace Regexxer

#endif

