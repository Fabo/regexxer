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

#include "undostack.h"
#include <glib.h>


namespace Regexxer
{

/**** Regexxer::UndoAction *************************************************/

UndoAction::~UndoAction()
{}

bool UndoAction::undo()
{
  return do_undo();
}


/**** Regexxer::UndoStack **************************************************/

UndoStack::UndoStack()
{}

UndoStack::~UndoStack()
{}

void UndoStack::push(const UndoActionPtr& action)
{
  actions_.push(action);
}

bool UndoStack::empty() const
{
  return actions_.empty();
}

void UndoStack::undo_step()
{
  bool skip = false;

  do
  {
    g_return_if_fail(!actions_.empty());

    const UndoActionPtr action = actions_.top();
    actions_.pop();

    skip = action->undo();
  }
  while(skip);
}

bool UndoStack::do_undo()
{
  bool skip = true;

  while(!actions_.empty())
  {
    const UndoActionPtr action = actions_.top();
    actions_.pop();

    skip = (action->undo() && skip);
  }

  return skip;
}

} // namespace Regexxer

