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

const char regexxer_project_url[]   = "http://regexxer.sourceforge.net/";
const char regexxer_author_mail[]   = "Daniel Elstner <daniel.elstner@gmx.net>";
const char regexxer_debian_mail[]   = "Ross Burton <ross@burtonini.com>";


class SelectableLabel : public Gtk::Label
{
public:
  SelectableLabel(const Glib::ustring& label);
  virtual ~SelectableLabel();

protected:
  virtual bool on_focus(Gtk::DirectionType direction);
};

SelectableLabel::SelectableLabel(const Glib::ustring& label)
:
  Gtk::Label(label)
{
  set_selectable(true);
}

SelectableLabel::~SelectableLabel()
{}

bool SelectableLabel::on_focus(Gtk::DirectionType)
{
  if(can_focus() && !is_focus())
  {
    grab_focus();
    return true;
  }

  return false;
}


class ContributorBox : public Gtk::VBox
{
public:
  ContributorBox(const Glib::ustring& what, const Glib::ustring& who);
  virtual ~ContributorBox();
};

ContributorBox::ContributorBox(const Glib::ustring& what, const Glib::ustring& who)
:
  Gtk::VBox(false, 2)
{
  using namespace Gtk;

  Label *const label_what = new Label();
  pack_start(*manage(label_what), PACK_SHRINK);
  label_what->set_markup("<span size=\"small\">" + what + "</span>");

  pack_start(*manage(new SelectableLabel(who)), PACK_SHRINK);
}

ContributorBox::~ContributorBox()
{}

} // anonymous namespace


namespace Regexxer
{

AboutDialog::AboutDialog(Gtk::Window& parent)
:
  Gtk::Dialog("About regexxer", parent, false, true)
{
  using namespace Gtk;

  add_button(Stock::OK, RESPONSE_OK)->grab_focus();

  Box& box_dialog = *get_vbox();
  Alignment *const alignment = new Alignment(0.5, 0.33, 0.0, 0.0);
  box_dialog.pack_start(*manage(alignment), PACK_EXPAND_WIDGET);
  alignment->set_border_width(20);

  Box *const box = new VBox(false, 20);
  alignment->add(*manage(box));

  {
    Box *const box_title = new HBox(false, 10);
    box->pack_start(*manage(box_title), PACK_SHRINK);

    Image *const image = new Image(regexxer_icon_filename);
    box_title->pack_start(*manage(image), PACK_EXPAND_WIDGET);
    image->set_alignment(1.0, 0.5);

    Label *const label_title = new Label();
    box_title->pack_start(*manage(label_title), PACK_EXPAND_WIDGET);
    label_title->set_alignment(0.0, 0.5);
    label_title->set_markup("<span size=\"xx-large\" weight=\"heavy\">" PACKAGE_STRING "</span>");
  }
  {
    Box *const box_text = new VBox(false, 10);
    box->pack_start(*manage(box_text), PACK_SHRINK);

    Label *const label_url = new Label(regexxer_project_url);
    box_text->pack_start(*manage(label_url), PACK_SHRINK, 5);
    label_url->set_selectable(true);

    Widget *const box_author = new ContributorBox("written by", regexxer_author_mail);
    box_text->pack_start(*manage(box_author), PACK_SHRINK);

    Widget *const box_debian = new ContributorBox("Debian package by", regexxer_debian_mail);
    box_text->pack_start(*manage(box_debian), PACK_SHRINK);
  }

  alignment->show_all();

  set_default_response(RESPONSE_OK);
}

AboutDialog::~AboutDialog()
{}

void AboutDialog::on_response(int)
{
  hide();
}

} // namespace Regexxer

