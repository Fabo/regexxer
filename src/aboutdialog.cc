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

#include <glib.h>
#include <gtkmm/alignment.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/stock.h>

#include <config.h>


namespace
{

const char regexxer_icon_filename[] = REGEXXER_DATADIR G_DIR_SEPARATOR_S
                                      "pixmaps" G_DIR_SEPARATOR_S "regexxer.png";

const char regexxer_author_list[]   = "Daniel Elstner <daniel.elstner@gmx.net>";
const char regexxer_project_url[]   = "http://regexxer.sourceforge.net/";

} // anonymous namespace


namespace Regexxer
{

AboutDialog::AboutDialog(Gtk::Window& parent)
:
  Gtk::Dialog("About regexxer", parent, false, true)
{
  using namespace Gtk;

  add_button(Stock::OK, RESPONSE_OK);

  Box& box_dialog = *get_vbox();
  Alignment *const alignment = new Alignment(0.5, 0.33, 0.0, 0.0);
  box_dialog.pack_start(*manage(alignment), PACK_EXPAND_WIDGET);

  Box *const box = new VBox(false, 10);
  alignment->add(*manage(box));
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
  label_title->set_markup("<span size=\"xx-large\" weight=\"heavy\">" PACKAGE_STRING "</span>");

  Label *const label_mail = new Label(regexxer_author_list);
  box->pack_start(*manage(label_mail), PACK_SHRINK);
  label_mail->set_selectable(true);

  Label *const label_url = new Label(regexxer_project_url);
  box->pack_start(*manage(label_url), PACK_SHRINK);
  label_url->set_selectable(true);

  alignment->show_all();
}

AboutDialog::~AboutDialog()
{}

void AboutDialog::on_response(int)
{
  hide();
}

} // namespace Regexxer

