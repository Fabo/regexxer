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
