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

#include "signalutils.h"


namespace Util
{

QueuedSignal::QueuedSignal(int priority)
:
  signal_   (),
  priority_ (priority),
  queued_   (false)
{}

QueuedSignal::~QueuedSignal()
{}

SigC::Connection QueuedSignal::connect(const SigC::Slot0<void>& slot)
{
  return signal_.connect(slot);
}

void QueuedSignal::queue()
{
  if(!queued_)
  {
    Glib::signal_idle().connect(SigC::slot(*this, &QueuedSignal::idle_handler), priority_);
    queued_ = true;
  }
}

bool QueuedSignal::idle_handler()
{
  queued_ = false;
  signal_(); // emit

  return false; // disconnect idle handler
}

} // namespace Util

