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

#ifndef REGEXXER_STRINGUTILS_H_INCLUDED
#define REGEXXER_STRINGUTILS_H_INCLUDED

#include <string>
#include <vector>
#include <glibmm/ustring.h>


namespace Util
{

typedef std::vector< std::pair<int,int> > CaptureVector;

// next() and prev(): Idea shamelessly stolen from boost.
//
template <class Iterator>
inline Iterator next(Iterator pos) { return ++pos; }

template <class Iterator>
inline Iterator prev(Iterator pos) { return --pos; }

bool encodings_equal(const std::string& lhs, const std::string& rhs);
bool contains_null(const char* pbegin, const char* pend);
Glib::ustring shell_pattern_to_regex(const Glib::ustring& pattern);

std::string substitute_references(const std::string&   substitution,
                                  const std::string&   subject,
                                  const CaptureVector& captures);

Glib::ustring filename_to_utf8_fallback(const std::string& filename);
Glib::ustring convert_to_ascii(const std::string& str);
Glib::ustring int_to_string(int number);

Glib::ustring transform_pathname(const Glib::ustring& path, bool shorten);

inline Glib::ustring shorten_pathname(const Glib::ustring& path)
  { return transform_pathname(path, true); }

inline Glib::ustring expand_pathname(const Glib::ustring& path)
  { return transform_pathname(path, false); }

} // namespace Util

#endif /* REGEXXER_STRINGUTILS_H_INCLUDED */

