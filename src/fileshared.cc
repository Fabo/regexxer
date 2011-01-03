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

#include "fileshared.h"

#include <glib.h>
#include <algorithm>


namespace
{

static
const Glib::Quark& file_buffer_match_quark()
{
  // Regexxer::FileBuffer uses anonymous Gtk::TextMark objects to remember
  // the position of matches.  This quark is used to identify a match mark,
  // in order to be able to distinguish it from other anonymous marks.

  static Glib::Quark quark ("regexxer-file-buffer-match-quark");
  return quark;
}

static
int calculate_match_length(const Glib::ustring& subject, const std::pair<int, int>& bounds)
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

MatchData::MatchData(int match_index, const Glib::ustring& line,
                     Glib::MatchInfo& match_info)
:
  index   (match_index),
  subject (line)
{
  int capture_count = match_info.get_match_count();
  captures.reserve(capture_count);

  for (int i = 0; i < capture_count; ++i)
  {
    std::pair<int, int> bounds;
    match_info.fetch_pos(i, bounds.first, bounds.second);
    captures.push_back(bounds);
  }

  length = calculate_match_length(subject, captures.front());
}

MatchData::~MatchData()
{
  // We *should* be the only one holding a reference to the Mark apart from
  // the Gtk::TextBuffer itself.  Nonetheless, let's better be extra careful
  // and invalidate the reference.
  if (mark)
    mark->steal_data(file_buffer_match_quark());
}

void MatchData::install_mark(const Gtk::TextBuffer::iterator& pos)
{
  g_return_if_fail(!mark);

  mark = pos.get_buffer()->create_mark(pos, false); // right gravity

  mark->set_data(file_buffer_match_quark(), this);
}

// static
bool MatchData::is_match_mark(const Glib::RefPtr<Gtk::TextMark>& textmark)
{
  return (textmark->get_data(file_buffer_match_quark()) != 0);
}

// static
Util::SharedPtr<MatchData> MatchData::get_from_mark(const Glib::RefPtr<Gtk::TextMark>& textmark)
{
  return MatchDataPtr(static_cast<MatchData*>(textmark->get_data(file_buffer_match_quark())));
}

} // namespace Regexxer
