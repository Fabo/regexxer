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

#include "aboutdialog.h"
#include "globalstrings.h"

#include <glibmm/markup.h>
#include <gtkmm/dialog.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <libglademm.h>

#include <config.h>


namespace
{

const char *const package_title = "<span size=\"xx-large\" weight=\"heavy\">"
                                  PACKAGE_STRING "</span>";

void apply_label_markup(Gtk::Label& label)
{
  label.set_markup("<span size=\"small\">" + Glib::Markup::escape_text(label.get_text()) + "</span>");
}

} // anonymous namespace


namespace Regexxer
{

std::auto_ptr<Gtk::Dialog> AboutDialog::create(Gtk::Window& parent)
{
  using Gnome::Glade::Xml;

  const Glib::RefPtr<Xml> xml = Xml::create(glade_aboutdialog_filename);

  Gtk::Dialog* aboutdialog = 0;
  std::auto_ptr<Gtk::Dialog> dialog (xml->get_widget("aboutdialog", aboutdialog));

  dialog->set_transient_for(parent);
  dialog->signal_response().connect(sigc::hide(sigc::mem_fun(*dialog, &Gtk::Widget::hide)));

  Gtk::Image* image = 0;
  xml->get_widget("image", image)->set(application_icon_filename);

  Gtk::Label* label = 0;
  xml->get_widget("label_title", label)->set_markup(package_title);
  apply_label_markup(*xml->get_widget("label_author_what", label));
  apply_label_markup(*xml->get_widget("label_translator_what", label));

  return dialog;
}

} // namespace Regexxer

