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

#include "prefdialog.h"

#include <glib.h>
#include <gtkmm.h>
#include <memory>

#include <config.h>


namespace
{

const char regexxer_icon_filename[] = REGEXXER_DATADIR G_DIR_SEPARATOR_S
                                      "pixmaps" G_DIR_SEPARATOR_S "regexxer.png";

const char regexxer_author_list[]   = "Daniel Elstner <daniel.elstner@gmx.net>";
const char regexxer_project_url[]   = "http://regexxer.sourceforge.net/";


class ImageLabel : public Gtk::HBox
{
public:
  ImageLabel(const Gtk::StockID& stock_id, const Glib::ustring& label);
  virtual ~ImageLabel();
};

ImageLabel::ImageLabel(const Gtk::StockID& stock_id, const Glib::ustring& label)
:
  Gtk::HBox(false, 2)
{
  pack_start(*Gtk::manage(new Gtk::Image(stock_id, Gtk::ICON_SIZE_MENU)), Gtk::PACK_SHRINK);
  pack_start(*Gtk::manage(new Gtk::Label(label)), Gtk::PACK_SHRINK);

  show_all_children();
}

ImageLabel::~ImageLabel()
{}

} // anonymous namespace


namespace Regexxer
{

PrefDialog::PrefDialog(Gtk::Window& parent)
:
  Gtk::Dialog("Preferences", parent),
  button_icons_       (0),
  button_text_        (0),
  button_both_        (0),
  button_both_horiz_  (0),
  entry_fallback_     (0)
{
  using namespace Gtk;

  add_button(Stock::CLOSE, RESPONSE_CLOSE);

  Box *const box = get_vbox();
  Notebook *const notebook = new Notebook();
  box->pack_start(*manage(notebook));

  {
    std::auto_ptr<Widget> label_options (new ImageLabel(Stock::PROPERTIES, "Options"));
    Gtk::Widget *const page_options = create_page_options();

    notebook->append_page(*manage(page_options), *manage(label_options.release()));
  }
  {
    std::auto_ptr<Widget> label_info (new ImageLabel(StockID("regexxer-info"), "Info"));
    Gtk::Widget *const page_info = create_page_info();

    notebook->append_page(*manage(page_info), *manage(label_info.release()));
  }

  notebook->show_all();
}

PrefDialog::~PrefDialog()
{}

void PrefDialog::set_pref_toolbar_style(Gtk::ToolbarStyle toolbar_style)
{
  switch(toolbar_style)
  {
    case Gtk::TOOLBAR_ICONS:      button_icons_     ->set_active(true); break;
    case Gtk::TOOLBAR_TEXT:       button_text_      ->set_active(true); break;
    case Gtk::TOOLBAR_BOTH:       button_both_      ->set_active(true); break;
    case Gtk::TOOLBAR_BOTH_HORIZ: button_both_horiz_->set_active(true); break;
    default:
      g_return_if_reached();
  }
}

void PrefDialog::on_response(int)
{
  hide();
}

Gtk::Widget* PrefDialog::create_page_options()
{
  using namespace Gtk;

  std::auto_ptr<Box> page (new VBox(false, 0));

  {
    Box *const box_toolbar = new HBox(false, 5);
    page->pack_start(*manage(box_toolbar), PACK_EXPAND_PADDING);
    box_toolbar->set_border_width(10);

    box_toolbar->pack_start(*manage(new Label("Toolbar style:", 0.0, 0.5)), PACK_SHRINK);

    Box *const box_radio = new VBox(true, 0);
    box_toolbar->pack_start(*manage(box_radio), PACK_SHRINK);

    RadioButton::Group radio_group;

    button_icons_ = new RadioButton(radio_group, "_Icons only", true);
    box_radio->pack_start(*manage(button_icons_), PACK_SHRINK);

    button_text_ = new RadioButton(radio_group, "_Text only", true);
    box_radio->pack_start(*manage(button_text_), PACK_SHRINK);

    button_both_ = new RadioButton(radio_group, "Icons _and text", true);
    box_radio->pack_start(*manage(button_both_), PACK_SHRINK);

    button_both_horiz_ = new RadioButton(radio_group, "Both _horizontal", true);
    box_radio->pack_start(*manage(button_both_horiz_), PACK_SHRINK);

    const SigC::Slot0<void> slot_toggled (SigC::slot(*this, &PrefDialog::on_radio_toggled));

    button_icons_     ->signal_toggled().connect(slot_toggled);
    button_text_      ->signal_toggled().connect(slot_toggled);
    button_both_      ->signal_toggled().connect(slot_toggled);
    button_both_horiz_->signal_toggled().connect(slot_toggled);
  }
  {
    Box *const box_encoding = new VBox(false, 10);
    page->pack_start(*manage(box_encoding), PACK_EXPAND_PADDING);
    box_encoding->set_border_width(10);

    Label *const label_info = new Label("regexxer attempts to read a file in the following "
                                        "encodings before giving up:", 0.0, 0.5);
    box_encoding->pack_start(*manage(label_info), PACK_SHRINK);
    label_info->set_line_wrap(true);

    Table *const table = new Table(3, 2, false);
    box_encoding->pack_start(*manage(table), PACK_SHRINK);
    table->set_row_spacings(2);
    table->set_col_spacings(3);

    table->attach(*manage(new Label("1.", 1.0, 0.5)), 0, 1, 0, 1, FILL, FILL);
    table->attach(*manage(new Label("2.", 1.0, 0.5)), 0, 1, 1, 2, FILL, FILL);
    table->attach(*manage(new Label("3.", 1.0, 0.5)), 0, 1, 2, 3, FILL, FILL);
    table->attach(*manage(new Label("UTF-8", 0.0, 0.5)), 1, 2, 0, 1, FILL, FILL);
    table->attach(*manage(new Label("The encoding specified by the current locale", 0.0, 0.5)),
                  1, 2, 1, 2, FILL, FILL);

    Box *const box_fallback = new HBox(false, 5);
    table->attach(*manage(box_fallback), 1, 2, 2, 3, EXPAND|FILL, FILL);

    Label *const label_fallback = new Label("Fallback _encoding:", 0.0, 0.5, true);
    box_fallback->pack_start(*manage(label_fallback), PACK_SHRINK);

    entry_fallback_ = new Entry();
    box_fallback->pack_start(*manage(entry_fallback_), PACK_EXPAND_WIDGET);

    label_fallback->set_mnemonic_widget(*entry_fallback_);
  }

  return page.release();
}

Gtk::Widget* PrefDialog::create_page_info()
{
  using namespace Gtk;

  std::auto_ptr<Alignment> page (new Alignment(0.5, 0.33, 0.0, 0.0));

  Box *const box = new VBox(false, 10);
  page->add(*manage(box));
  box->set_border_width(10);

  Box *const box_title = new HBox(false, 10);
  box->pack_start(*manage(box_title), PACK_SHRINK);
  box_title->set_border_width(10);

  Image *const image = new Image(regexxer_icon_filename);
  box_title->pack_start(*manage(image), PACK_EXPAND_WIDGET);
  image->set_alignment(1.0, 0.5);

  Label *const label_title = new Label();
  box_title->pack_start(*manage(label_title), PACK_EXPAND_WIDGET);
  label_title->set_alignment(0.0, 0.5);
  label_title->set_markup("<span size=\"xx-large\" weight=\"heavy\">"
                          PACKAGE_NAME " " PACKAGE_VERSION "</span>");

  Label *const label_mail = new Label(regexxer_author_list);
  box->pack_start(*manage(label_mail), PACK_SHRINK);
  label_mail->set_selectable(true);

  Label *const label_url = new Label(regexxer_project_url);
  box->pack_start(*manage(label_url), PACK_SHRINK);
  label_url->set_selectable(true);

  return page.release();
}

void PrefDialog::on_radio_toggled()
{
  const Gtk::ToolbarStyle toolbar_style =
      (button_icons_->get_active() ? Gtk::TOOLBAR_ICONS :
      (button_text_ ->get_active() ? Gtk::TOOLBAR_TEXT  :
      (button_both_ ->get_active() ? Gtk::TOOLBAR_BOTH  : Gtk::TOOLBAR_BOTH_HORIZ)));

  signal_pref_toolbar_style_changed(toolbar_style); // emit
}

} // namespace Regexxer

