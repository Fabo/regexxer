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

#include "mainwindow.h"

#include <glib.h>
#include <gtkmm/iconfactory.h>
#include <gtkmm/iconset.h>
#include <gtkmm/iconsource.h>
#include <gtkmm/main.h>
#include <gtkmm/stock.h>
#include <gtkmm/stockitem.h>

#include <exception>
#include <list>


namespace
{

#include <pixmaps/stockimages.h>

struct StockData
{
  const char*   id;
  const guint8* icondata;
  unsigned int  iconsize;
  const char*   label;
};

const StockData regexxer_stock_items[] =
{
  { "regexxer-info",     stock_menu_about,  sizeof(stock_menu_about),  "Info"      },
  { "regexxer-save-all", stock_save_all_24, sizeof(stock_save_all_24), "Save _all" }
};

void regexxer_register_stock_items()
{
  const Glib::RefPtr<Gtk::IconFactory> factory = Gtk::IconFactory::create();

  for(unsigned i = 0; i < G_N_ELEMENTS(regexxer_stock_items); ++i)
  {
    const StockData& stock = regexxer_stock_items[i];
    const Gtk::StockID stock_id (stock.id);

    const Gtk::IconSet icon_set (Gdk::Pixbuf::create_from_inline(stock.iconsize, stock.icondata));
    factory->add(stock_id, icon_set);
    Gtk::Stock::add(Gtk::StockItem(stock_id, stock.label));
  }

  factory->add_default();
}

void regexxer_set_window_icon()
{
  const char *const regexxer_icon_filename =
      REGEXXER_DATADIR G_DIR_SEPARATOR_S "pixmaps" G_DIR_SEPARATOR_S "regexxer.png";

  try
  {
    std::list< Glib::RefPtr<Gdk::Pixbuf> > icons;
    icons.push_back(Gdk::Pixbuf::create_from_file(regexxer_icon_filename));
    Gtk::Window::set_default_icon_list(icons);
  }
  catch(const Glib::Error& error)
  {
    const Glib::ustring what = error.what();
    g_warning(what.c_str());
  }
}

} // anonymous namespace


int main(int argc, char** argv)
{
  try
  {
    Gtk::Main main_instance (&argc, &argv);

    regexxer_register_stock_items();
    regexxer_set_window_icon();

    Regexxer::MainWindow window;
    Gtk::Main::run(window);
  }
  catch(const Glib::Error& error)
  {
    const Glib::ustring what = error.what();
    g_error("unhandled exception: %s", what.c_str());
  }
  catch(const std::exception& except)
  {
    g_error("unhandled exception: %s", except.what());
  }
  catch(...)
  {
    g_error("unhandled exception: type unknown");
  }

  return 0;
}

