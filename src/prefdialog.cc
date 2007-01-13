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

#include <glib.h>
#include <gtkmm.h>
#include <gconfmm/client.h>
#include <libglademm/xml.h>
#include <list>

#include <config.h>

namespace
{

static
const Gtk::ToolbarStyle toolbar_style_values[] =
{
  Gtk::TOOLBAR_ICONS,
  Gtk::TOOLBAR_TEXT,
  Gtk::TOOLBAR_BOTH,
  Gtk::TOOLBAR_BOTH_HORIZ
};

static
int get_toolbar_style_index(const Glib::ustring& value)
{
  const Gtk::ToolbarStyle toolbar_style = Util::enum_from_nick<Gtk::ToolbarStyle>(value);

  for (unsigned int i = 0; i < G_N_ELEMENTS(toolbar_style_values); ++i)
  {
    if (toolbar_style_values[i] == toolbar_style)
      return i;
  }

  g_return_val_if_reached(-1);
}

} // anonymous namespace

namespace Regexxer
{

/**** Regexxer::PrefDialog *************************************************/

PrefDialog::PrefDialog(Gtk::Window& parent)
:
  dialog_                 (),
  button_textview_font_   (0),
  button_match_color_     (0),
  button_current_color_   (0),
  combo_toolbar_style_    (0),
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
  using Gnome::Glade::Xml;

  const Glib::RefPtr<Xml> xml = Xml::create(glade_prefdialog_filename);

  Dialog* prefdialog = 0;
  dialog_.reset(xml->get_widget("prefdialog", prefdialog));

  xml->get_widget("button_textview_font", button_textview_font_);
  xml->get_widget("button_match_color",   button_match_color_);
  xml->get_widget("button_current_color", button_current_color_);
  xml->get_widget("combo_toolbar_style",  combo_toolbar_style_);
  xml->get_widget("entry_fallback",       entry_fallback_);

  const Glib::RefPtr<SizeGroup> size_group = SizeGroup::create(SIZE_GROUP_VERTICAL);

  size_group->add_widget(*xml->get_widget("label_utf8"));
  size_group->add_widget(*xml->get_widget("label_locale"));
  size_group->add_widget(*xml->get_widget("box_fallback"));
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

  conn_toolbar_style_ = combo_toolbar_style_->signal_changed().connect(
      sigc::mem_fun(*this, &PrefDialog::on_option_toolbar_style_changed));

  entry_fallback_->signal_changed().connect(
      sigc::mem_fun(*this, &PrefDialog::on_entry_fallback_changed));

  entry_fallback_->signal_activate().connect(
      sigc::mem_fun(*this, &PrefDialog::on_entry_fallback_activate));
}

void PrefDialog::on_response(int)
{
  if (entry_fallback_changed_)
    entry_fallback_->activate();

  Gnome::Conf::Client::get_default_client()->suggest_sync();

  dialog_->hide();
}

/*
 * Note that it isn't strictly required to block the change notification
 * as done below for the "toolbar_style" setting.  GConf doesn't emit
 * "value_changed" if the new value is identical to the old one.  If, however,
 * the value was reset to the schema default, the following change notification
 * would again detach the schema.  This won't look neat, and I like neat.
 */
void PrefDialog::on_conf_value_changed(const Glib::ustring& key, const Gnome::Conf::Value& value)
{
  if (value.get_type() == Gnome::Conf::VALUE_STRING)
  {
    if (key.raw() == conf_key_textview_font)
    {
      button_textview_font_->set_font_name(value.get_string());
    }
    else if (key.raw() == conf_key_match_color)
    {
      button_match_color_->set_color(Gdk::Color(value.get_string()));
    }
    else if (key.raw() == conf_key_current_match_color)
    {
      button_current_color_->set_color(Gdk::Color(value.get_string()));
    }
    else if (key.raw() == conf_key_toolbar_style)
    {
      Util::ScopedBlock block (conn_toolbar_style_);
      combo_toolbar_style_->set_active(get_toolbar_style_index(value.get_string()));
    }
    else if (key.raw() == conf_key_fallback_encoding)
    {
      entry_fallback_->set_text(value.get_string());
      entry_fallback_changed_ = false;
    }
  }
}

void PrefDialog::initialize_configuration()
{
  using namespace Gnome::Conf;

  const Glib::RefPtr<Client> client = Client::get_default_client();
  const std::list<Entry> entries (client->all_entries(conf_dir_application));

  for (std::list<Entry>::const_iterator p = entries.begin(); p != entries.end(); ++p)
  {
    on_conf_value_changed(p->get_key(), p->get_value());
  }

  client->signal_value_changed().connect(
      sigc::mem_fun(*this, &PrefDialog::on_conf_value_changed));
}

void PrefDialog::on_textview_font_set()
{
  const Glib::ustring value = button_textview_font_->get_font_name();
  Gnome::Conf::Client::get_default_client()->set(conf_key_textview_font, value);
}

void PrefDialog::on_match_color_set()
{
  const Glib::ustring value = Util::color_to_string(button_match_color_->get_color());
  Gnome::Conf::Client::get_default_client()->set(conf_key_match_color, value);
}

void PrefDialog::on_current_color_set()
{
  const Glib::ustring value = Util::color_to_string(button_current_color_->get_color());
  Gnome::Conf::Client::get_default_client()->set(conf_key_current_match_color, value);
}

void PrefDialog::on_option_toolbar_style_changed()
{
  const int index = combo_toolbar_style_->get_active_row_number();

  if (index >= 0)
  {
    g_return_if_fail(unsigned(index) < G_N_ELEMENTS(toolbar_style_values));

    const Glib::ustring value = Util::enum_to_nick(toolbar_style_values[index]);
    Gnome::Conf::Client::get_default_client()->set(conf_key_toolbar_style, value);
  }
}

void PrefDialog::on_entry_fallback_changed()
{
  // On dialog close, write back to the GConf database only if the user
  // actually did something with the entry widget.  This prevents GConf from
  // detaching the key's Schema each time the preferences dialog is closed.
  entry_fallback_changed_ = true;
}

void PrefDialog::on_entry_fallback_activate()
{
  const Glib::ustring fallback_encoding = entry_fallback_->get_text();

  if (Util::validate_encoding(fallback_encoding.raw()))
  {
    Gnome::Conf::Client::get_default_client()
        ->set(conf_key_fallback_encoding, fallback_encoding.uppercase());

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
