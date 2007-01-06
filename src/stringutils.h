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

#ifndef REGEXXER_STRINGUTILS_H_INCLUDED
#define REGEXXER_STRINGUTILS_H_INCLUDED

#include <glibmm/ustring.h>
#include <glibmm/value.h>

#include <string>
#include <vector>

namespace Gdk { class Color; }


namespace Util
{

typedef std::vector< std::pair<int,int> > CaptureVector;

bool validate_encoding(const std::string& encoding);
bool encodings_equal(const std::string& lhs, const std::string& rhs);
Glib::ustring shell_pattern_to_regex(const Glib::ustring& pattern);

Glib::ustring substitute_references(const Glib::ustring& substitution,
                                    const Glib::ustring& subject,
                                    const CaptureVector& captures);

Glib::ustring filename_to_utf8_fallback(const std::string& filename);
Glib::ustring convert_to_ascii(const std::string& str);
Glib::ustring int_to_string(int number);

std::string shorten_pathname(const std::string& path);
std::string expand_pathname(const std::string& path);

Glib::ustring color_to_string(const Gdk::Color& color);

int enum_from_nick_impl(GType type, const Glib::ustring& nick);
Glib::ustring enum_to_nick_impl(GType type, int value);

template <class T> inline
T enum_from_nick(const Glib::ustring& nick)
  { return static_cast<T>(Util::enum_from_nick_impl(Glib::Value<T>::value_type(), nick)); }

template <class T> inline
Glib::ustring enum_to_nick(T value)
  { return Util::enum_to_nick_impl(Glib::Value<T>::value_type(), value); }

} // namespace Util

#endif /* REGEXXER_STRINGUTILS_H_INCLUDED */
