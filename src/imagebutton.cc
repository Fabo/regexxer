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

#include "imagebutton.h"

#include <atkmm.h>
#include <gtkmm/alignment.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>

#include <config.h>


namespace Regexxer
{

ImageButton::ImageButton(const Gtk::StockID& stock_id, const Glib::ustring& name)
{
  Gtk::Image *const image = new Gtk::Image(stock_id, Gtk::ICON_SIZE_BUTTON);
  add(*Gtk::manage(image));
  image->show();

#if REGEXXER_HAVE_GTKMM_22
  get_accessible()->set_name(name);
#else
  (void) name; // suppress warning about unused parameter
#endif
}

ImageButton::~ImageButton()
{}


ImageLabelButton::ImageLabelButton(const Gtk::StockID& stock_id, const Glib::ustring& label,
                                   bool mnemonic)
{
  using namespace Gtk;

  Alignment *const alignment = new Alignment(0.5, 0.5, 0.0, 0.0);
  add(*manage(alignment));

  HBox *const hbox = new HBox(false, 3);
  alignment->add(*manage(hbox));

  hbox->pack_start(*manage(new Image(stock_id, ICON_SIZE_BUTTON)), PACK_SHRINK);
  hbox->pack_start(*manage(new Label(label, mnemonic)),            PACK_SHRINK);

  show_all_children();
}

ImageLabelButton::~ImageLabelButton()
{}

} // namespace Regexxer

