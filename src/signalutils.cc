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

/**** Util::QueuedSignal ***************************************************/

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

/**** Util::AutoConnection *************************************************/

AutoConnection::AutoConnection()
:
  connection_ (),
  blocked_    (false)
{}

AutoConnection::AutoConnection(const SigC::Connection& connection)
:
  connection_ (connection),
  blocked_    (connection_.blocked())
{}

AutoConnection::~AutoConnection()
{
  connection_.disconnect();
}

void AutoConnection::block()
{
  connection_.block();
  blocked_ = true;
}

void AutoConnection::unblock()
{
  connection_.unblock();
  blocked_ = false;
}

AutoConnection& AutoConnection::operator=(const SigC::Connection& connection)
{
  AutoConnection temp (connection_);

  connection_ = connection;
  connection_.block(blocked_);

  return *this;
}

void AutoConnection::disconnect()
{
  connection_.disconnect();
}

} // namespace Util

