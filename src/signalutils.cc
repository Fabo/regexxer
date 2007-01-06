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

sigc::connection QueuedSignal::connect(const sigc::slot<void>& slot)
{
  return signal_.connect(slot);
}

void QueuedSignal::queue()
{
  if (!queued_)
  {
    Glib::signal_idle().connect(sigc::mem_fun(*this, &QueuedSignal::idle_handler), priority_);
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

AutoConnection::AutoConnection(const sigc::connection& connection)
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

AutoConnection& AutoConnection::operator=(const sigc::connection& connection)
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
