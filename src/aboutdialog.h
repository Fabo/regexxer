/* $Id$
 *
 * Copyright (c) 2004  Daniel Elstner  <daniel.elstner@gmx.net>
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

#ifndef REGEXXER_ABOUTDIALOG_H_INCLUDED
#define REGEXXER_ABOUTDIALOG_H_INCLUDED

#include <memory>

namespace Gtk
{
class Dialog;
class Window;
}


namespace Regexxer
{

namespace AboutDialog
{
  std::auto_ptr<Gtk::Dialog> create(Gtk::Window& parent);
}

} // namespace Regexxer

#endif /* REGEXXER_ABOUTDIALOG_H_INCLUDED */

