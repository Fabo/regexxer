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

#include "filebufferundo.h"
#include "filebuffer.h"

#include <glib.h>


namespace Regexxer
{

/**** Regexxer::FileBufferActionInsert *************************************/

FileBufferActionInsert::FileBufferActionInsert(FileBuffer& filebuffer, int offset,
                                               const Glib::ustring& text)
:
  FileBufferAction(filebuffer),
  text_           (text),
  offset_         (offset)
{
  buffer().increment_stamp();
}

FileBufferActionInsert::~FileBufferActionInsert()
{}

bool FileBufferActionInsert::do_undo(const sigc::slot<bool>&)
{
  g_return_val_if_fail(!buffer().in_user_action(), false);

  const FileBuffer::iterator text_begin = buffer().get_iter_at_offset(offset_);

  FileBuffer::iterator text_end = text_begin;
  text_end.forward_chars(text_.length());

  text_end = buffer().erase(text_begin, text_end);
  buffer().place_cursor(text_end);

  buffer().decrement_stamp();

  return false;
}


/**** Regexxer::FileBufferActionErase **************************************/

FileBufferActionErase::FileBufferActionErase(FileBuffer& filebuffer, int offset,
                                             const Glib::ustring& text)
:
  FileBufferAction(filebuffer),
  text_           (text),
  offset_         (offset)
{
  buffer().increment_stamp();
}

FileBufferActionErase::~FileBufferActionErase()
{}

bool FileBufferActionErase::do_undo(const sigc::slot<bool>&)
{
  g_return_val_if_fail(!buffer().in_user_action(), false);

  FileBuffer::iterator pos = buffer().get_iter_at_offset(offset_);

  pos = buffer().insert(pos, text_);
  buffer().place_cursor(pos);

  buffer().decrement_stamp();

  return false;
}


/**** Regexxer::FileBufferActionRemoveMatch ********************************/

FileBufferActionRemoveMatch::FileBufferActionRemoveMatch(
    FileBuffer& filebuffer, int offset, const MatchDataPtr& match)
:
  FileBufferAction(filebuffer),
  match_          (match),
  offset_         (offset)
{
  if (match_)
    buffer().undo_add_weak(this);
}

FileBufferActionRemoveMatch::~FileBufferActionRemoveMatch()
{
  if (match_)
    buffer().undo_remove_weak(this);
}

void FileBufferActionRemoveMatch::weak_notify()
{
  match_.reset();
}

bool FileBufferActionRemoveMatch::do_undo(const sigc::slot<bool>&)
{
  if (match_)
  {
    g_return_val_if_fail(!buffer().in_user_action(), true);

    buffer().undo_remove_match(match_, offset_);
  }
  return true;
}

} // namespace Regexxer
