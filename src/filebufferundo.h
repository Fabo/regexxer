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

#ifndef REGEXXER_FILEBUFFERUNDO_H_INCLUDED
#define REGEXXER_FILEBUFFERUNDO_H_INCLUDED

#include "undostack.h"
#include "fileshared.h"


namespace Regexxer
{

class FileBuffer;

class FileBufferAction : public UndoAction
{
public:
  explicit FileBufferAction(FileBuffer& filebuffer) : buffer_ (filebuffer) {}

protected:
  FileBuffer& buffer() { return buffer_; }

private:
  FileBuffer& buffer_;
};

class FileBufferActionInsert : public FileBufferAction
{
public:
  FileBufferActionInsert(FileBuffer& filebuffer, int offset, const Glib::ustring& text);
  virtual ~FileBufferActionInsert();

private:
  Glib::ustring text_;
  int           offset_;

  virtual bool do_undo();
};

class FileBufferActionErase : public FileBufferAction
{
public:
  FileBufferActionErase(FileBuffer& filebuffer, int offset, const Glib::ustring& text);
  virtual ~FileBufferActionErase();

private:
  Glib::ustring text_;
  int           offset_;

  virtual bool do_undo();
};

class FileBufferActionRemoveMatch : public FileBufferAction
{
public:
  FileBufferActionRemoveMatch(FileBuffer& filebuffer, int offset, const MatchDataPtr& match);
  virtual ~FileBufferActionRemoveMatch();

private:
  MatchDataPtr  match_;
  int           offset_;

  virtual bool do_undo();
};

} // namespace Regexxer

#endif /* REGEXXER_FILEBUFFERUNDO_H_INCLUDED */

