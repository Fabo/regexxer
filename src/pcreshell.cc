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

#include "pcreshell.h"

#include <glib.h>
#include <glibmm.h>


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
  pcre_       (0),
  pcre_extra_ (0),
  ovector_    (0),
  ovecsize_   (0)
{
  const char* error_message = 0;
  int         error_offset  = -1;

  pcre_ = pcre_compile(regex.c_str(), options | PCRE_UTF8, &error_message, &error_offset, 0);

  if(!pcre_)
  {
    g_assert(error_message != 0);
    throw Error(Glib::locale_to_utf8(error_message), error_offset);
  }

  pcre_extra_ = pcre_study(pcre_, 0, &error_message);

  if(error_message)
  {
    (*pcre_free)(pcre_);
    throw Error(Glib::locale_to_utf8(error_message));
  }

  int capture_count = 0;
  const int rc = pcre_fullinfo(pcre_, pcre_extra_, PCRE_INFO_CAPTURECOUNT, &capture_count);

  g_assert(rc == 0);
  g_assert(capture_count >= 0);

  ovecsize_ = 3 * (capture_count + 1);
  ovector_  = g_new(int, ovecsize_);
}

Pattern::~Pattern()
{
  (*pcre_free)(pcre_);
  (*pcre_free)(pcre_extra_);
  g_free(ovector_);
}

int Pattern::match(const Glib::ustring& subject, int offset, MatchOptions options)
{
  return pcre_exec(pcre_, pcre_extra_,
                   subject.data(), subject.bytes(), offset,
                   options, ovector_, ovecsize_);
}

std::pair<int,int> Pattern::get_substring_bounds(int index) const
{
  return std::pair<int,int>(ovector_[2 * index], ovector_[2 * index + 1]);
}

Glib::ustring Pattern::get_substring(const Glib::ustring& subject, int index) const
{
  const int begin = ovector_[2 * index];
  const int end   = ovector_[2 * index + 1];

  if(begin >= 0 && end > begin)
  {
    const char *const data = subject.data();
    return Glib::ustring(data + begin, data + end);
  }

  return Glib::ustring();
}

} // namespace Pcre

