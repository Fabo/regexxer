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

#include "prefdialog.h"
#include "globalstrings.h"
#include "stringutils.h"
#include "translation.h"

#include <glib.h>
#include <gconfmm.h>
#include <gtkmm.h>
#include <libglademm.h>
#include <list>

#include <config.h>


namespace
{

const Gtk::ToolbarStyle toolbar_style_values[] =
{
  Gtk::TOOLBAR_ICONS,
  Gtk::TOOLBAR_TEXT,
  Gtk::TOOLBAR_BOTH,
  Gtk::TOOLBAR_BOTH_HORIZ
};

unsigned int get_toolbar_style_index(const Glib::ustring& value)
{
  const Gtk::ToolbarStyle toolbar_style = Util::enum_from_nick<Gtk::ToolbarStyle>(value);

  for (unsigned int i = 0; i < G_N_ELEMENTS(toolbar_style_values); ++i)
  {
    if (toolbar_style_values[i] == toolbar_style)
      return i;
  }

  g_return_val_if_reached(0);
}

} // anonymous namespace


namespace Regexxer
{

/**** Regexxer::FontSelectionButton ****************************************/

class FontSelectionButton : public Gtk::Button
{
public:
  FontSelectionButton();
  virtual ~FontSelectionButton();

  void set_selected_font(const Glib::ustring& font_name);
  Glib::ustring get_selected_font() const;

  sigc::signal<void> signal_font_selected;

protected:
  virtual void on_clicked();

private:
  Gtk::Label* label_font_;
  Gtk::Label* label_size_;
};

FontSelectionButton::FontSelectionButton()
:
  label_font_ (0),
  label_size_ (0)
{
  using namespace Gtk;

  Box *const box = new HBox(false, 0);
  add(*manage(box));

  box->pack_start(*manage(label_font_ = new Label()), PACK_EXPAND_WIDGET, 6 /* HIG */);
  box->pack_start(*manage(new VSeparator()),          PACK_SHRINK);
  box->pack_start(*manage(label_size_ = new Label()), PACK_SHRINK, 6 /* HIG */);
}

FontSelectionButton::~FontSelectionButton()
{}

void FontSelectionButton::set_selected_font(const Glib::ustring& font_name)
{
  const Pango::FontDescription font (font_name);

  label_font_->set_text(font.get_family());
  label_size_->set_text(Util::int_to_string((font.get_size() + Pango::SCALE / 2) / Pango::SCALE));

  label_font_->modify_font(font);

  get_accessible()->set_name(font.to_string());
}

Glib::ustring FontSelectionButton::get_selected_font() const
{
  return label_font_->get_pango_context()->get_font_description().to_string();
}

void FontSelectionButton::on_clicked()
{
  Gtk::FontSelectionDialog dialog;

  dialog.set_modal(true);

  if (Gtk::Window *const toplevel = dynamic_cast<Gtk::Window*>(get_toplevel()))
    dialog.set_transient_for(*toplevel);

  dialog.set_font_name(get_selected_font());

  if (dialog.run() == Gtk::RESPONSE_OK)
  {
    set_selected_font(dialog.get_font_name());
    signal_font_selected(); // emit
  }
}

extern "C"
GtkWidget* regexxer_create_font_selection_button(char*, char*, char*, int, int)
{
  try
  {
    Gtk::Widget *const widget = new FontSelectionButton();
    widget->show_all();
    return Gtk::manage(widget)->gobj();
  }
  catch (...)
  {
    g_return_val_if_reached(0);
  }
}


/**** Regexxer::ColorSelectionButton ***************************************/

class ColorSelectionButton : public Gtk::Button
{
public:
  ColorSelectionButton();
  virtual ~ColorSelectionButton();

  void set_selected_color(const Gdk::Color& color);
  Gdk::Color get_selected_color() const;

  sigc::signal<void> signal_color_selected;

protected:
  virtual void on_clicked();

private:
  class ColorLabel : public Gtk::Label
  {
  public:
    ColorLabel();
    virtual ~ColorLabel();

    void adapt_activity_colors();

  protected:
    virtual void on_realize();
    virtual bool on_expose_event(GdkEventExpose* event);
  };

  ColorLabel* colorlabel_;
};


ColorSelectionButton::ColorSelectionButton()
{
  add(*Gtk::manage(colorlabel_ = new ColorLabel()));
}

ColorSelectionButton::~ColorSelectionButton()
{}

void ColorSelectionButton::set_selected_color(const Gdk::Color& color)
{
  colorlabel_->modify_bg(Gtk::STATE_NORMAL, color);

  if (colorlabel_->is_realized())
    colorlabel_->adapt_activity_colors();

  colorlabel_->set_text(Util::color_to_string(color));
}

Gdk::Color ColorSelectionButton::get_selected_color() const
{
  return colorlabel_->get_style()->get_bg(Gtk::STATE_NORMAL);
}

void ColorSelectionButton::on_clicked()
{
  Gtk::ColorSelectionDialog dialog;

  dialog.set_modal(true);
  dialog.set_title(_("Color Selection"));

  if (Gtk::Window *const toplevel = dynamic_cast<Gtk::Window*>(get_toplevel()))
    dialog.set_transient_for(*toplevel);

  Gtk::ColorSelection& colorsel = *dialog.get_colorsel();

  colorsel.set_has_palette(true);
  colorsel.set_current_color(get_selected_color());

  if (dialog.run() == Gtk::RESPONSE_OK)
  {
    set_selected_color(colorsel.get_current_color());
    signal_color_selected(); // emit
  }
}

ColorSelectionButton::ColorLabel::ColorLabel()
{
  const Glib::RefPtr<Gtk::RcStyle> rcstyle = get_modifier_style();
  const Glib::ustring none = "<none>";

  rcstyle->set_bg_pixmap_name(Gtk::STATE_NORMAL,   none);
  rcstyle->set_bg_pixmap_name(Gtk::STATE_ACTIVE,   none);
  rcstyle->set_bg_pixmap_name(Gtk::STATE_PRELIGHT, none);

  modify_style(rcstyle);
}

ColorSelectionButton::ColorLabel::~ColorLabel()
{}

void ColorSelectionButton::ColorLabel::adapt_activity_colors()
{
  const Glib::RefPtr<Gtk::Style> style = get_style();

  modify_bg(Gtk::STATE_ACTIVE,   style->get_mid  (Gtk::STATE_NORMAL));
  modify_bg(Gtk::STATE_PRELIGHT, style->get_light(Gtk::STATE_NORMAL));
}

void ColorSelectionButton::ColorLabel::on_realize()
{
  Gtk::Label::on_realize();
  adapt_activity_colors();
}

/*
 * Note that the Gtk::Label expose event handler is not called.  Thus the
 * label text is invisible, but we still provide accessibility information.
 * Displaying the hex color string in the button to all users is probably
 * a bad idea.
 */
bool ColorSelectionButton::ColorLabel::on_expose_event(GdkEventExpose* event)
{
  int focus_padding    = 0;
  int focus_line_width = 0;

  Gtk::Widget& button = *get_parent();

  button.get_style_property("focus_padding",    focus_padding);
  button.get_style_property("focus_line_width", focus_line_width);

  const int margin = focus_padding + focus_line_width;
  const Gtk::Allocation alloc = get_allocation();

  get_style()->paint_box(
      get_window(), get_state(), Gtk::SHADOW_ETCHED_OUT,
      Glib::wrap(&event->area), *this, "regexxer-color-label",
      alloc.get_x() + margin,
      alloc.get_y() + margin,
      std::max(0, alloc.get_width()  - 2 * margin),
      std::max(0, alloc.get_height() - 2 * margin));

  return false;
}

extern "C"
GtkWidget* regexxer_create_color_selection_button(char*, char*, char*, int, int)
{
  try
  {
    Gtk::Widget *const widget = new ColorSelectionButton();
    widget->show_all();
    return Gtk::manage(widget)->gobj();
  }
  catch (...)
  {
    g_return_val_if_reached(0);
  }
}


/**** Regexxer::PrefDialog *************************************************/

PrefDialog::PrefDialog(Gtk::Window& parent)
:
  dialog_                 (),
  button_textview_font_   (0),
  button_match_color_     (0),
  button_current_color_   (0),
  option_toolbar_style_   (0),
  entry_fallback_         (0),
  button_direction_       (0),
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
  xml->get_widget("option_toolbar_style", option_toolbar_style_);
  xml->get_widget("entry_fallback",       entry_fallback_);
  xml->get_widget("button_direction",     button_direction_);

  const Glib::RefPtr<SizeGroup> size_group = SizeGroup::create(SIZE_GROUP_VERTICAL);

  size_group->add_widget(*xml->get_widget("label_utf8"));
  size_group->add_widget(*xml->get_widget("label_locale"));
  size_group->add_widget(*xml->get_widget("box_fallback"));
}

void PrefDialog::connect_signals()
{
  dialog_->signal_response().connect(
      sigc::mem_fun(*this, &PrefDialog::on_response));

  button_textview_font_->signal_font_selected.connect(
      sigc::mem_fun(*this, &PrefDialog::on_textview_font_selected));

  button_match_color_->signal_color_selected.connect(
      sigc::mem_fun(*this, &PrefDialog::on_match_color_selected));

  button_current_color_->signal_color_selected.connect(
      sigc::mem_fun(*this, &PrefDialog::on_current_color_selected));

  conn_toolbar_style_ = option_toolbar_style_->signal_changed().connect(
      sigc::mem_fun(*this, &PrefDialog::on_option_toolbar_style_changed));

  entry_fallback_->signal_changed().connect(
      sigc::mem_fun(*this, &PrefDialog::on_entry_fallback_changed));

  entry_fallback_->signal_activate().connect(
      sigc::mem_fun(*this, &PrefDialog::on_entry_fallback_activate));

  conn_direction_ = button_direction_->signal_toggled().connect(
      sigc::mem_fun(*this, &PrefDialog::on_button_direction_toggled));
}

void PrefDialog::on_response(int)
{
  if (entry_fallback_changed_)
  {
    try // trap errors manually since we don't go through a GObject signal handler
    {
      on_entry_fallback_activate();
    }
    catch (const Gnome::Conf::Error&)
    {}
  }
  dialog_->hide();
}

void PrefDialog::on_conf_value_changed_hack(const Glib::ustring& key,
                                            const Gnome::Conf::Value& value)
{
  REGEXXER_GCONFMM_VALUE_HACK(value);
  on_conf_value_changed(key, value);
}

/*
 * Note that it isn't strictly required to block the change notifications
 * as done below for the "toolbar_style" and "override_direction" settings.
 * GConf doesn't emit "value_changed" if the new value is identical to the
 * old one.  If, however, the value was reset to the schema default, the
 * following change notification would again detach the schema.  This won't
 * look neat, and I like neat.
 */
void PrefDialog::on_conf_value_changed(const Glib::ustring& key, const Gnome::Conf::Value& value)
{
  if (value.get_type() == Gnome::Conf::VALUE_STRING)
  {
    if (key.raw() == conf_key_textview_font)
    {
      button_textview_font_->set_selected_font(value.get_string());
    }
    else if (key.raw() == conf_key_match_color)
    {
      button_match_color_->set_selected_color(Gdk::Color(value.get_string()));
    }
    else if (key.raw() == conf_key_current_match_color)
    {
      button_current_color_->set_selected_color(Gdk::Color(value.get_string()));
    }
    else if (key.raw() == conf_key_toolbar_style)
    {
      Util::ScopedBlock block (conn_toolbar_style_);
      option_toolbar_style_->set_history(get_toolbar_style_index(value.get_string()));
    }
    else if (key.raw() == conf_key_fallback_encoding)
    {
      entry_fallback_->set_text(value.get_string());
      entry_fallback_changed_ = false;
    }
  }
  else if (value.get_type() == Gnome::Conf::VALUE_BOOL)
  {
    if (key.raw() == conf_key_override_direction)
    {
      Util::ScopedBlock block (conn_direction_);
      button_direction_->set_active(value.get_bool());
    }
  }
}

void PrefDialog::initialize_configuration()
{
  using namespace Gnome::Conf;

  const Glib::RefPtr<Client> client = Client::get_default_client();
  const std::list<Entry> entries (client->all_entries(REGEXXER_GCONF_DIRECTORY));

  for (std::list<Entry>::const_iterator p = entries.begin(); p != entries.end(); ++p)
  {
    on_conf_value_changed(p->get_key(), p->get_value());
  }

  client->signal_value_changed().connect(
      sigc::mem_fun(*this, &PrefDialog::on_conf_value_changed_hack));
}

void PrefDialog::on_textview_font_selected()
{
  const Glib::ustring value = button_textview_font_->get_selected_font();
  Gnome::Conf::Client::get_default_client()->set(conf_key_textview_font, value);
}

void PrefDialog::on_match_color_selected()
{
  const Glib::ustring value = Util::color_to_string(button_match_color_->get_selected_color());
  Gnome::Conf::Client::get_default_client()->set(conf_key_match_color, value);
}

void PrefDialog::on_current_color_selected()
{
  const Glib::ustring value = Util::color_to_string(button_current_color_->get_selected_color());
  Gnome::Conf::Client::get_default_client()->set(conf_key_current_match_color, value);
}

void PrefDialog::on_option_toolbar_style_changed()
{
  const unsigned int option_index = option_toolbar_style_->get_history();

  g_return_if_fail(option_index < G_N_ELEMENTS(toolbar_style_values));

  const Glib::ustring value = Util::enum_to_nick(toolbar_style_values[option_index]);
  Gnome::Conf::Client::get_default_client()->set(conf_key_toolbar_style, value);
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
    const Glib::ustring message = Util::compose(_("\"%1\" is not a valid encoding."),
                                                fallback_encoding);
    Gtk::MessageDialog error_dialog (*dialog_, message, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);

    error_dialog.run();
  }
}

void PrefDialog::on_button_direction_toggled()
{
  const bool value = button_direction_->get_active();
  Gnome::Conf::Client::get_default_client()->set(conf_key_override_direction, value);
}

} // namespace Regexxer

