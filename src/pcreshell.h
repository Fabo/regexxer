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

#ifndef REGEXXER_PCRESHELL_H_INCLUDED
#define REGEXXER_PCRESHELL_H_INCLUDED

#include <glibmm/ustring.h>
#include <pcre.h>
#include <utility>


namespace Pcre
{

enum CompileOptions
{
  ANCHORED        = PCRE_ANCHORED,
  CASELESS        = PCRE_CASELESS,
  DOLLAR_ENDONLY  = PCRE_DOLLAR_ENDONLY,
  DOTALL          = PCRE_DOTALL,
  EXTENDED        = PCRE_EXTENDED,
  EXTRA           = PCRE_EXTRA,
  MULTILINE       = PCRE_MULTILINE,
  UNGREEDY        = PCRE_UNGREEDY
};

inline CompileOptions operator|(CompileOptions lhs, CompileOptions rhs)
  { return static_cast<CompileOptions>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs)); }

inline CompileOptions operator&(CompileOptions lhs, CompileOptions rhs)
  { return static_cast<CompileOptions>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs)); }

inline CompileOptions operator~(CompileOptions flags)
  { return static_cast<CompileOptions>(~static_cast<unsigned>(flags)); }


enum MatchOptions
{
  NOT_BOL   = PCRE_NOTBOL,
  NOT_EOL   = PCRE_NOTEOL,
  NOT_EMPTY = PCRE_NOTEMPTY
};

inline MatchOptions operator|(MatchOptions lhs, MatchOptions rhs)
  { return static_cast<MatchOptions>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs)); }

inline MatchOptions operator&(MatchOptions lhs, MatchOptions rhs)
  { return static_cast<MatchOptions>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs)); }

inline MatchOptions operator~(MatchOptions flags)
  { return static_cast<MatchOptions>(~static_cast<unsigned>(flags)); }


class Error
{
public:
  explicit Error(const Glib::ustring& message, int offset = -1);
  virtual ~Error();

  Error(const Error& other);
  Error& operator=(const Error& other);

  Glib::ustring what()   const { return message_; }
  int           offset() const { return offset_;  } // in characters

private:
  Glib::ustring message_;
  int           offset_;
};


/* Pcre::Pattern represents a compiled regular expression.
 * Note that all offset values are in bytes, not characters.
 */
class Pattern
{
public:
  explicit Pattern(const Glib::ustring& regex, CompileOptions options = CompileOptions(0));
  virtual ~Pattern();

  // takes byte offset
  int match(const Glib::ustring& subject, int offset = 0, MatchOptions options = MatchOptions(0));

  // returns byte offsets
  std::pair<int,int> get_substring_bounds(int index) const;
  Glib::ustring get_substring(const Glib::ustring& subject, int index) const;

private:
  pcre*       pcre_;
  pcre_extra* pcre_extra_;
  int*        ovector_;
  int         ovecsize_;

  Pattern(const Pattern&);
  Pattern& operator=(const Pattern&);
};

} // namespace Pcre

#endif /* REGEXXER_PCRESHELL_H_INCLUDED */

