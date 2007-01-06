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

#ifndef REGEXXER_SIGNALUTILS_H_INCLUDED
#define REGEXXER_SIGNALUTILS_H_INCLUDED

#include <glibmm.h>


namespace Util
{

class QueuedSignal : public sigc::trackable
{
public:
  explicit QueuedSignal(int priority = Glib::PRIORITY_HIGH_IDLE);
  virtual ~QueuedSignal();

  sigc::connection connect(const sigc::slot<void>& slot);
  void queue();
  void operator()() { queue(); }

private:
  sigc::signal<void>  signal_;
  int                 priority_;
  bool                queued_;

  QueuedSignal(const QueuedSignal&);
  QueuedSignal& operator=(const QueuedSignal&);

  bool idle_handler();
};

class AutoConnection
{
private:
  sigc::connection  connection_;
  bool              blocked_;

  AutoConnection(const AutoConnection&);
  AutoConnection& operator=(const AutoConnection&);

public:
  AutoConnection();
  explicit AutoConnection(const sigc::connection& connection);
  ~AutoConnection();

  void block();
  void unblock();
  bool blocked() const { return blocked_; }

  AutoConnection& operator=(const sigc::connection& connection);
  void disconnect();

  sigc::connection&       base()       { return connection_; }
  const sigc::connection& base() const { return connection_; }
};

class ScopedConnection
{
private:
  sigc::connection connection_;

  ScopedConnection(const ScopedConnection&);
  ScopedConnection& operator=(const ScopedConnection&);

public:
  explicit ScopedConnection(const sigc::connection& connection)
    : connection_ (connection) {}

  ~ScopedConnection() { connection_.disconnect(); }
};

class ScopedBlock
{
private:
  AutoConnection& connection_;

  ScopedBlock(const ScopedBlock&);
  ScopedBlock& operator=(const ScopedBlock&);

public:
  explicit ScopedBlock(AutoConnection& connection)
    : connection_ (connection) { connection_.block(); }

  ~ScopedBlock() { connection_.unblock(); }
};

} // namespace Util

#endif /* REGEXXER_SIGNALUTILS_H_INCLUDED */
