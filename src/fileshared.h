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

#ifndef REGEXXER_FILESHARED_H_INCLUDED
#define REGEXXER_FILESHARED_H_INCLUDED

#include "sharedptr.h"

#include <gtkmm/textbuffer.h>
#include <functional>
#include <utility>
#include <vector>

namespace Regexxer
{

enum BoundState
{
  BOUND_NONE  = 0,
  BOUND_FIRST = 1 << 0,
  BOUND_LAST  = 1 << 1
};

inline BoundState operator|(BoundState a, BoundState b)
  { return static_cast<BoundState>(static_cast<unsigned>(a) | static_cast<unsigned>(b)); }

inline BoundState operator&(BoundState a, BoundState b)
  { return static_cast<BoundState>(static_cast<unsigned>(a) & static_cast<unsigned>(b)); }

inline BoundState operator^(BoundState a, BoundState b)
  { return static_cast<BoundState>(static_cast<unsigned>(a) ^ static_cast<unsigned>(b)); }

inline BoundState operator~(BoundState flags)
  { return static_cast<BoundState>(~static_cast<unsigned>(flags)); }

inline BoundState& operator|=(BoundState& a, BoundState b)
  { return (a = static_cast<BoundState>(static_cast<unsigned>(a) | static_cast<unsigned>(b))); }

inline BoundState& operator&=(BoundState& a, BoundState b)
  { return (a = static_cast<BoundState>(static_cast<unsigned>(a) & static_cast<unsigned>(b))); }

inline BoundState& operator^=(BoundState& a, BoundState b)
  { return (a = static_cast<BoundState>(static_cast<unsigned>(a) ^ static_cast<unsigned>(b))); }

/*
 * This struct holds all the information that's necessary to locate a
 * match's position in the buffer and to substitute captured substrings
 * into a replacement string.  The latter is achieved by storing the whole
 * subject string (i.e. the line in the buffer) together with a table of
 * indices into it.  This arrangement should consume less memory than the
 * alternative of storing a vector of captured substrings since we want
 * to support $&, $`, $' too.
 */
struct MatchData : public Util::SharedObject
{
  int                                index;
  int                                length;
  Glib::ustring                      subject;
  std::vector< std::pair<int, int> > captures;
  Glib::RefPtr<Gtk::TextMark>        mark;

  MatchData(int match_index, const Glib::ustring& line,
            Glib::MatchInfo& match_info);
  ~MatchData();

  void install_mark(const Gtk::TextBuffer::iterator& pos);

  static bool                       is_match_mark(const Glib::RefPtr<Gtk::TextMark>& textmark);
  static Util::SharedPtr<MatchData> get_from_mark(const Glib::RefPtr<Gtk::TextMark>& textmark);

private:
  MatchData(const MatchData&);
  MatchData& operator=(const MatchData&);
};

typedef Util::SharedPtr<MatchData> MatchDataPtr;

/*
 * Sort predicate for use with std::set<>.
 */
struct MatchDataLess : public std::binary_function<MatchDataPtr, MatchDataPtr, bool>
{
  bool operator()(const MatchDataPtr& a, const MatchDataPtr& b) const
    { return (a->index < b->index); }
};

} // namespace Regexxer

#endif /* REGEXXER_FILESHARED_H_INCLUDED */
