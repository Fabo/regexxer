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

bool FileBufferActionInsert::do_undo()
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

bool FileBufferActionErase::do_undo()
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
{}

FileBufferActionRemoveMatch::~FileBufferActionRemoveMatch()
{}

bool FileBufferActionRemoveMatch::do_undo()
{
  g_return_val_if_fail(!buffer().in_user_action(), true);

  buffer().undo_remove_match(match_, offset_);

  return true;
}

} // namespace Regexxer

