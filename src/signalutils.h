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

#ifndef REGEXXER_SIGNALUTILS_H_INCLUDED
#define REGEXXER_SIGNALUTILS_H_INCLUDED

#include <glibmm.h>


namespace Util
{

class QueuedSignal : public SigC::Object
{
public:
  explicit QueuedSignal(int priority = Glib::PRIORITY_HIGH_IDLE);
  virtual ~QueuedSignal();

  SigC::Connection connect(const SigC::Slot0<void>& slot);
  void queue();
  void operator()() { queue(); }

private:
  SigC::Signal0<void> signal_;
  int                 priority_;
  bool                queued_;

  QueuedSignal(const QueuedSignal&);
  QueuedSignal& operator=(const QueuedSignal&);

  bool idle_handler();
};

} // namespace Util

#endif /* REGEXXER_SIGNALUTILS_H_INCLUDED */

