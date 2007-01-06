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

#include "aboutdialog.h"
#include "globalstrings.h"

#include <glibmm/markup.h>
#include <gtkmm/dialog.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <libglademm/xml.h>

#include <config.h>


namespace
{

const char *const package_title = "<span size=\"xx-large\" weight=\"heavy\">"
                                  PACKAGE_STRING "</span>";

void apply_label_what_markup(Gtk::Label& label)
{
  label.set_markup("<small>" + Glib::Markup::escape_text(label.get_text()) + "</small>");
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
  xml->get_widget("image", image)->set_from_icon_name("regexxer", Gtk::ICON_SIZE_DIALOG);

  Gtk::Label* label = 0;
  xml->get_widget("label_title", label)->set_markup(package_title);
  apply_label_what_markup(*xml->get_widget("label_author_what", label));
  apply_label_what_markup(*xml->get_widget("label_translator_what", label));

  if (xml->get_widget("label_translator_who", label)->get_text().raw() == "translator-credits")
    label->set_markup("<i>(no translation available)</i>");

  return dialog;
}

} // namespace Regexxer
