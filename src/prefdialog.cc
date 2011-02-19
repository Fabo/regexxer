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

#include "prefdialog.h"
#include "globalstrings.h"
#include "stringutils.h"
#include "translation.h"
#include "settings.h"

#include <glib.h>
#include <gtkmm.h>
#include <list>

#include <config.h>

namespace Regexxer
{

/**** Regexxer::PrefDialog *************************************************/

PrefDialog::PrefDialog(Gtk::Window& parent)
:
  dialog_                 (),
  button_textview_font_   (0),
  button_match_color_     (0),
  button_current_color_   (0),
  entry_fallback_         (0),
  entry_fallback_changed_ (false)
{
  load_xml();
  dialog_->set_transient_for(parent);

  initialize_configuration();
  connect_signals();
}

PrefDialog::~PrefDialog()
{}

void PrefDialog::load_xml()
{
  using namespace Gtk;
  
  const Glib::RefPtr<Builder> xml = Builder::create_from_file(ui_prefdialog_filename);

  Dialog* prefdialog = 0;
  xml->get_widget("prefdialog", prefdialog);
  dialog_.reset(prefdialog);

  xml->get_widget("button_textview_font", button_textview_font_);
  xml->get_widget("button_match_color",   button_match_color_);
  xml->get_widget("button_current_color", button_current_color_);
  xml->get_widget("entry_fallback",       entry_fallback_);

  const Glib::RefPtr<SizeGroup> size_group = SizeGroup::create(SIZE_GROUP_VERTICAL);

  Label* label_utf8 = 0, *label_locale = 0;
  Box* box_fallback = 0;
  
  xml->get_widget("label_utf8",   label_utf8);
  xml->get_widget("label_locale", label_locale);
  xml->get_widget("box_fallback", box_fallback);
  
  size_group->add_widget(*label_utf8);
  size_group->add_widget(*label_locale);
  size_group->add_widget(*box_fallback);
}

void PrefDialog::connect_signals()
{
  dialog_->signal_response().connect(
      sigc::mem_fun(*this, &PrefDialog::on_response));

  button_textview_font_->signal_font_set().connect(
      sigc::mem_fun(*this, &PrefDialog::on_textview_font_set));

  button_match_color_->signal_color_set().connect(
      sigc::mem_fun(*this, &PrefDialog::on_match_color_set));

  button_current_color_->signal_color_set().connect(
      sigc::mem_fun(*this, &PrefDialog::on_current_color_set));

  entry_fallback_->signal_changed().connect(
      sigc::mem_fun(*this, &PrefDialog::on_entry_fallback_changed));

  entry_fallback_->signal_activate().connect(
      sigc::mem_fun(*this, &PrefDialog::on_entry_fallback_activate));
}

void PrefDialog::on_response(int)
{
  if (entry_fallback_changed_)
    entry_fallback_->activate();

  dialog_->hide();
}

void PrefDialog::on_conf_value_changed(const Glib::ustring& key)
{
  Glib::RefPtr<Gio::Settings> settings = Settings::instance();

  if (key.raw() == conf_key_textview_font)
  {
    button_textview_font_->set_font_name(settings->get_string(key));
  }
  else if (key.raw() == conf_key_match_color)
  {
    button_match_color_->set_color(Gdk::Color(settings->get_string(key)));
  }
  else if (key.raw() == conf_key_current_match_color)
  {
    button_current_color_->set_color(Gdk::Color(settings->get_string(key)));
  }
  else if (key.raw() == conf_key_fallback_encoding)
  {
    entry_fallback_->set_text(settings->get_string(key));
    entry_fallback_changed_ = false;
  }
}

void PrefDialog::initialize_configuration()
{
  Glib::RefPtr<Gio::Settings> settings = Settings::instance();
  const std::list<Glib::ustring> entries = settings->list_keys();

  for (std::list<Glib::ustring>::const_iterator p = entries.begin(); p != entries.end(); ++p)
    on_conf_value_changed(*p);

  settings->bind(conf_key_textview_font, button_textview_font_, "font_name");
}

void PrefDialog::on_textview_font_set()
{
  const Glib::ustring value = button_textview_font_->get_font_name();
  Settings::instance()->set_string(conf_key_textview_font, value);
}

void PrefDialog::on_match_color_set()
{
  const Glib::ustring value = Util::color_to_string(button_match_color_->get_color());
  Settings::instance()->set_string(conf_key_match_color, value);
}

void PrefDialog::on_current_color_set()
{
  const Glib::ustring value = Util::color_to_string(button_current_color_->get_color());
  Settings::instance()->set_string(conf_key_current_match_color, value);
}

void PrefDialog::on_entry_fallback_changed()
{
  entry_fallback_changed_ = true;
}

void PrefDialog::on_entry_fallback_activate()
{
  const Glib::ustring fallback_encoding = entry_fallback_->get_text();

  if (Util::validate_encoding(fallback_encoding.raw()))
  {
    Settings::instance()->set_string(conf_key_fallback_encoding,
                                     fallback_encoding.uppercase());

    entry_fallback_changed_ = false;
  }
  else
  {
    const Glib::ustring message =
        Util::compose(_("\342\200\234%1\342\200\235 is not a valid encoding."), fallback_encoding);

    Gtk::MessageDialog error_dialog (*dialog_, message, false,
                                     Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
    error_dialog.run();
  }
}

} // namespace Regexxer
