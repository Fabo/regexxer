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

#ifndef REGEXXER_FILEBUFFERUNDO_H_INCLUDED
#define REGEXXER_FILEBUFFERUNDO_H_INCLUDED

#include "undostack.h"
#include "fileshared.h"


namespace Regexxer
{

class FileBuffer;

class FileBufferAction : public UndoAction
{
protected:
  explicit FileBufferAction(FileBuffer& filebuffer)
    : buffer_ (filebuffer) {}

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

  virtual bool do_undo(const sigc::slot<bool>& pulse);
};

class FileBufferActionErase : public FileBufferAction
{
public:
  FileBufferActionErase(FileBuffer& filebuffer, int offset, const Glib::ustring& text);
  virtual ~FileBufferActionErase();

private:
  Glib::ustring text_;
  int           offset_;

  virtual bool do_undo(const sigc::slot<bool>& pulse);
};

class FileBufferActionRemoveMatch : public FileBufferAction
{
public:
  FileBufferActionRemoveMatch(FileBuffer& filebuffer, int offset, const MatchDataPtr& match);
  virtual ~FileBufferActionRemoveMatch();

  void weak_notify();

private:
  MatchDataPtr  match_;
  int           offset_;

  virtual bool do_undo(const sigc::slot<bool>& pulse);
};

} // namespace Regexxer

#endif /* REGEXXER_FILEBUFFERUNDO_H_INCLUDED */
