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

#include "miscutils.h"
#include "signalutils.h"

#include <glib-object.h>
#include <gconf/gconf-value.h>
#include <gconfmm.h>

#include <config.h>

#if REGEXXER_ENABLE_GCONFMM_VALUE_HACK

namespace
{

const void* received_value_pointer = 0;
bool        broken_value_changed   = false;

void value_changed_handler(const Glib::ustring&, const Gnome::Conf::Value& value)
{
  received_value_pointer = value.gobj();
}

} // anonymous namespace


void Util::check_for_broken_gconfmm_value_changed()
{
  const Glib::RefPtr<Gnome::Conf::Client> client = Gnome::Conf::Client::get_default_client();
  Util::AutoConnection connection (client->signal_value_changed().connect(&value_changed_handler));

  GConfValue* value = gconf_value_new(GCONF_VALUE_INT);
  gconf_value_set_int(value, 0);

  g_signal_emit_by_name(client->gobj(), "value_changed", "dummy_key", value);

  broken_value_changed = (received_value_pointer == value);

  if (!broken_value_changed)
    gconf_value_free(value);
}

bool Util::has_broken_gconfmm_value_changed()
{
  return broken_value_changed;
}

#endif /* REGEXXER_ENABLE_GCONFMM_VALUE_HACK */

