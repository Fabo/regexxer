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

#include "undostack.h"
#include <glib.h>


namespace
{

enum { PULSE_INTERVAL = 8 };

class StopUndo {};

} // anonymous namespace


namespace Regexxer
{

/**** Regexxer::UndoAction *************************************************/

UndoAction::~UndoAction()
{}

bool UndoAction::undo(const sigc::slot<bool>& pulse)
{
  return do_undo(pulse);
}


/**** Regexxer::UndoStack **************************************************/

UndoStack::UndoStack()
{}

UndoStack::~UndoStack()
{
  // Ensure LIFO order on destruction too.
  while (!actions_.empty())
    actions_.pop();
}

void UndoStack::push(const UndoActionPtr& action)
{
  actions_.push(action);
}

bool UndoStack::empty() const
{
  return actions_.empty();
}

void UndoStack::undo_step(const sigc::slot<bool>& pulse)
{
  try
  {
    bool skip = false;

    do
    {
      g_return_if_fail(!actions_.empty());

      skip = actions_.top()->undo(pulse);
      actions_.pop();
    }
    while (skip);
  }
  catch (const StopUndo&)
  {}
}

bool UndoStack::do_undo(const sigc::slot<bool>& pulse)
{
  unsigned int iteration = 0;
  bool skip = true;

  while (!actions_.empty())
  {
    const bool skip_this = actions_.top()->undo(pulse);
    actions_.pop();

    if (!skip_this)
    {
      skip = false;

      if ((++iteration % PULSE_INTERVAL) == 0 && pulse())
        throw StopUndo();
    }
  }

  return skip;
}

} // namespace Regexxer
