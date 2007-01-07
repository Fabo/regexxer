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

#include "globalstrings.h"
#include "mainwindow.h"
#include "miscutils.h"
#include "translation.h"

#include <glib.h>
#include <gtk/gtkwindow.h> /* for gtk_window_set_default_icon_name() */
#include <glibmm.h>
#include <gconfmm.h>
#include <gtkmm/iconfactory.h>
#include <gtkmm/iconset.h>
#include <gtkmm/iconsource.h>
#include <gtkmm/main.h>
#include <gtkmm/stock.h>
#include <gtkmm/stockitem.h>
#include <gtkmm/window.h>

#include <exception>
#include <list>

#include <config.h>

namespace
{

/*
 * Include inlined raw pixbuf data generated by gdk-pixbuf-csource.
 */
#include <ui/stockimages.h>

struct StockIconData
{
  const guint8*         data;
  int                   length;
  Gtk::BuiltinIconSize  size;
};

struct StockItemData
{
  const char*           id;
  const StockIconData*  icons;
  int                   n_icons;
  const char*           label;
};

class RegexxerOptions
{
private:
  std::auto_ptr<Regexxer::InitState>  init_state_;
  Glib::OptionGroup                   group_;
  Glib::OptionContext                 context_;

  static Glib::OptionEntry entry(const char* long_name, char short_name,
                                 const char* description, const char* arg_description = 0);
  RegexxerOptions();

public:
  static std::auto_ptr<RegexxerOptions> create();
  ~RegexxerOptions();

  Glib::OptionContext& context() { return context_; }
  std::auto_ptr<Regexxer::InitState> take_init_state() { return init_state_; }
};

const StockIconData stock_icon_save_all[] =
{
  { stock_save_all_16, sizeof(stock_save_all_16), Gtk::ICON_SIZE_MENU          },
  { stock_save_all_24, sizeof(stock_save_all_24), Gtk::ICON_SIZE_SMALL_TOOLBAR }
};

const StockItemData regexxer_stock_items[] =
{
  { "regexxer-save-all", stock_icon_save_all, G_N_ELEMENTS(stock_icon_save_all), N_("Save _all") }
};

const char *const locale_directory = REGEXXER_DATADIR G_DIR_SEPARATOR_S "locale";

// static
Glib::OptionEntry RegexxerOptions::entry(const char* long_name, char short_name,
                                         const char* description, const char* arg_description)
{
  Glib::OptionEntry option_entry;

  option_entry.set_long_name(long_name);
  option_entry.set_short_name(short_name);

  if (description)
    option_entry.set_description(description);
  if (arg_description)
    option_entry.set_arg_description(arg_description);

  return option_entry;
}

RegexxerOptions::RegexxerOptions()
:
  init_state_ (new Regexxer::InitState()),
  group_      (PACKAGE_TARNAME, Glib::ustring()),
  context_    ()
{}

RegexxerOptions::~RegexxerOptions()
{}

// static
std::auto_ptr<RegexxerOptions> RegexxerOptions::create()
{
  std::auto_ptr<RegexxerOptions> options (new RegexxerOptions());

  Glib::OptionGroup&   group = options->group_;
  Regexxer::InitState& init  = *options->init_state_;

  group.add_entry(entry("pattern", 'p', N_("Find files matching PATTERN"), N_("PATTERN")),
                  init.pattern);
  group.add_entry(entry("no-recursion", 'R', N_("Do not recurse into subdirectories")),
                  init.no_recursive);
  group.add_entry(entry("hidden", 'h', N_("Also find hidden files")),
                  init.hidden);
  group.add_entry(entry("regex", 'e', N_("Find text matching REGEX"), N_("REGEX")),
                  init.regex);
  group.add_entry(entry("no-global", 'G', N_("Find only the first match in a line")),
                  init.no_global);
  group.add_entry(entry("ignore-case", 'i', N_("Do case insensitive matching")),
                  init.ignorecase);
  group.add_entry(entry("substitution", 's', N_("Replace matches with STRING"), N_("STRING")),
                  init.substitution);
  group.add_entry(entry("line-number", 'n', N_("Print match location to standard output")),
                  init.feedback);
  group.add_entry(entry("no-autorun", 'A', N_("Do not automatically start search")),
                  init.no_autorun);
  group.add_entry_filename(entry(G_OPTION_REMAINING, '\0', 0, N_("[FOLDER]")),
                           init.folder);

  group.set_translation_domain(PACKAGE_TARNAME);
  options->context_.set_main_group(group);

  return options;
}

void register_stock_items()
{
  const Glib::RefPtr<Gtk::IconFactory> factory = Gtk::IconFactory::create();
  const Glib::ustring domain = PACKAGE_TARNAME;

  for (unsigned int item = 0; item < G_N_ELEMENTS(regexxer_stock_items); ++item)
  {
    const StockItemData& stock = regexxer_stock_items[item];
    Gtk::IconSet icon_set;

    for (int icon = 0; icon < stock.n_icons; ++icon)
    {
      const StockIconData& icon_data = stock.icons[icon];
      Gtk::IconSource source;

      source.set_pixbuf(Gdk::Pixbuf::create_from_inline(icon_data.length, icon_data.data));
      source.set_size(icon_data.size);

      // Unset wildcarded for all but the the last icon.
      source.set_size_wildcarded(icon == stock.n_icons - 1);

      icon_set.add_source(source);
    }

    const Gtk::StockID stock_id (stock.id);

    factory->add(stock_id, icon_set);
    Gtk::Stock::add(Gtk::StockItem(stock_id, stock.label, Gdk::ModifierType(0), 0, domain));
  }

  factory->add_default();
}

void trap_gconf_exceptions()
{
  try
  {
    throw; // re-throw current exception
  }
  catch (const Gnome::Conf::Error&)
  {
    // Ignore GConf exceptions thrown from GObject signal handlers.
    // GConf itself is going print the warning message for us
    // since we set the error handling mode to CLIENT_HANDLE_ALL.
  }
}

void initialize_configuration()
{
  using namespace Gnome::Conf;

  Glib::add_exception_handler(&trap_gconf_exceptions);

  const Glib::RefPtr<Client> client = Client::get_default_client();

  client->set_error_handling(CLIENT_HANDLE_ALL);
  client->add_dir(REGEXXER_GCONF_DIRECTORY, CLIENT_PRELOAD_ONELEVEL);

  const std::list<Entry> entries (client->all_entries(REGEXXER_GCONF_DIRECTORY));

  // Issue an artificial value_changed() signal for each entry in /apps/regexxer.
  // Reusing the signal handlers this way neatly avoids the need for separate
  // startup-initialization routines.

  for (std::list<Entry>::const_iterator p = entries.begin(); p != entries.end(); ++p)
  {
    client->value_changed(p->get_key(), p->get_value());
  }
}

} // anonymous namespace

int main(int argc, char** argv)
{
  try
  {
    Util::initialize_gettext(PACKAGE_TARNAME, locale_directory);
    Gnome::Conf::init();

    std::auto_ptr<RegexxerOptions> options = RegexxerOptions::create();
    Gtk::Main main_instance (argc, argv, options->context());
    Glib::set_application_name(PACKAGE_NAME);

    register_stock_items();
    gtk_window_set_default_icon_name(PACKAGE_TARNAME);

    Regexxer::MainWindow window;

    initialize_configuration();
    window.initialize(options->take_init_state());
    options.reset();

    Gtk::Main::run(*window.get_window());
  }
  catch (const Glib::OptionError& error)
  {
    const Glib::ustring what = error.what();
    g_printerr(PACKAGE_TARNAME ": %s\n", what.c_str());
    return 1;
  }
  catch (const Glib::Error& error)
  {
    const Glib::ustring what = error.what();
    g_error("unhandled exception: %s", what.c_str());
  }
  catch (const std::exception& ex)
  {
    g_error("unhandled exception: %s", ex.what());
  }
  catch (...)
  {
    g_error("unhandled exception: (type unknown)");
  }

  return 0;
}
