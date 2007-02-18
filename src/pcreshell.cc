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

#include "pcreshell.h"
#include "miscutils.h"
#include "stringutils.h"
#include "translation.h"

#include <pcre.h>
#include <glib.h>
#include <glibmm.h>
#include <algorithm>

namespace
{

static
int byte_to_char_offset(Glib::ustring::const_iterator start, int byte_offset)
{
  return std::distance(start, Glib::ustring::const_iterator(start.base() + byte_offset));
}

/*
 * Explicitely forbid any usage of the \C escape to match a single byte.
 * Having Pcre::Pattern::match() return matches on partial UTF-8 characters
 * would make regexxer crash faster than you can say "illegal sequence!"
 */
static
void check_for_single_byte_escape(const Glib::ustring& regex)
{
  using std::string;

  string::size_type index = 0;

  while ((index = regex.raw().find("\\C", index, 2)) != string::npos)
  {
    // Find the first of a sequence of backslashes preceding 'C'.
    string::size_type rewind = regex.raw().find_last_not_of('\\', index);
    rewind = (rewind != string::npos) ? rewind + 1 : 0;

    // The \C we found is a real \C escape only if preceded by an even number
    // of backslashes.  If this holds true, let's stage a right little tantrum.
    if ((index - rewind) % 2 == 0)
    {
      throw Pcre::Error(_("Using the \\C escape sequence to match a single byte is not supported."),
                        byte_to_char_offset(regex.begin(), index + 1));
    }
    index += 2;
  }
}

static
void throw_regex_error(const Glib::ustring&, int, const char*) G_GNUC_NORETURN;
static
void throw_regex_error(const Glib::ustring& regex, int byte_offset, const char* message)
{
  using Glib::ustring;

  const ustring what = (message) ? Glib::locale_to_utf8(message) : Glib::ustring();

  if (byte_offset >= 0 && unsigned(byte_offset) < regex.raw().size())
  {
    const int offset = byte_to_char_offset(regex.begin(), byte_offset);
    const gunichar error_char = *ustring::const_iterator(regex.raw().begin() + byte_offset);

    throw Pcre::Error(Util::compose(
        _("Error in regular expression at \342\200\234%1\342\200\235 (index %2):\n%3"),
        ustring(1, error_char), Util::int_to_string(offset + 1), what), offset);
  }
  else
  {
    throw Pcre::Error(Util::compose(_("Error in regular expression:\n%1"), what));
  }
}

} // anonymous namespace

namespace Pcre
{

/**** Pcre::Error **********************************************************/

Error::Error(const Glib::ustring& message, int offset)
:
  message_ (message),
  offset_  (offset)
{}

Error::~Error()
{}

Error::Error(const Error& other)
:
  message_ (other.message_),
  offset_  (other.offset_)
{}

Error& Error::operator=(const Error& other)
{
  // Note that this is exception safe because only the first assignment below
  // could throw.  If that changes the copy-and-swap technique should be used
  // instead.
  message_ = other.message_;
  offset_  = other.offset_;

  return *this;
}

/**** Pcre::Pattern ********************************************************/

Pattern::Pattern(const Glib::ustring& regex, CompileOptions options)
:
  pcre_     (0),
  ovector_  (0),
  ovecsize_ (0)
{
  check_for_single_byte_escape(regex);

  const char* error_message = 0;
  int error_offset = -1;

  pcre_ = pcre_compile(regex.c_str(), options | PCRE_UTF8, &error_message, &error_offset, 0);

  if (!pcre_)
    throw_regex_error(regex, error_offset, error_message);

  int capture_count = 0;
  const int result G_GNUC_UNUSED = pcre_fullinfo(static_cast<pcre*>(pcre_), 0,
                                                 PCRE_INFO_CAPTURECOUNT, &capture_count);
  g_assert(result == 0);
  g_assert(capture_count >= 0);

  ovecsize_ = 3 * (capture_count + 1);
  ovector_  = g_new0(int, ovecsize_);
}

Pattern::~Pattern()
{
  g_free(ovector_);
  (*pcre_free)(pcre_);
}

int Pattern::match(const Glib::ustring& subject, int offset, MatchOptions options)
{
  const int captures = pcre_exec(static_cast<pcre*>(pcre_), 0, subject.raw().data(),
                                 subject.raw().size(), offset, options, ovector_, ovecsize_);

  if (captures >= 0 || captures == PCRE_ERROR_NOMATCH)
    return captures;

  // Of all possible error conditions pcre_exec() might return, hitting
  // the match limit is the only one that could be triggered by user input.
  if (captures == PCRE_ERROR_MATCHLIMIT)
    throw Error(_("Reached the recursion and backtracking limit of the regular expression engine."));

  g_return_val_if_reached(captures);
}

std::pair<int, int> Pattern::get_substring_bounds(int index) const
{
  g_return_val_if_fail(index >= 0 && 3 * index < ovecsize_, std::make_pair(-1, -1));

  return std::make_pair(ovector_[2 * index], ovector_[2 * index + 1]);
}

Glib::ustring Pattern::get_substring(const Glib::ustring& subject, int index) const
{
  const std::pair<int, int> bounds = get_substring_bounds(index);

  if (bounds.first >= 0 && bounds.first < bounds.second)
  {
    const char *const data = subject.data();
    return Glib::ustring(data + bounds.first, data + bounds.second);
  }

  return Glib::ustring();
}

} // namespace Pcre
