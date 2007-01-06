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

#ifndef REGEXXER_UNDOSTACK_H_INCLUDED
#define REGEXXER_UNDOSTACK_H_INCLUDED

#include "sharedptr.h"

#include <sigc++/sigc++.h>
#include <stack>


namespace Regexxer
{

class UndoAction : public Util::SharedObject
{
public:
  UndoAction() {}
  virtual ~UndoAction() = 0;

  bool undo(const sigc::slot<bool>& pulse);

private:
  UndoAction(const UndoAction&);
  UndoAction& operator=(const UndoAction&);

  virtual bool do_undo(const sigc::slot<bool>& pulse) = 0;
};

typedef Util::SharedPtr<UndoAction> UndoActionPtr;


class UndoStack : public UndoAction
{
public:
  UndoStack();
  virtual ~UndoStack();

  void push(const UndoActionPtr& action);
  bool empty() const;

  void undo_step(const sigc::slot<bool>& pulse);

private:
  std::stack<UndoActionPtr> actions_;

  virtual bool do_undo(const sigc::slot<bool>& pulse);
};

typedef Util::SharedPtr<UndoStack> UndoStackPtr;


} // namespace Regexxer

#endif /* REGEXXER_UNDOSTACK_H_INCLUDED */
