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

#ifndef REGEXXER_PCRESHELL_H_INCLUDED
#define REGEXXER_PCRESHELL_H_INCLUDED

#include <glibmm/ustring.h>
#include <utility>


namespace Pcre
{

/*
 * The numeric values are copied from pcre.h.  This is quite safe
 * because they cannot be changed without breaking ABI.
 */
enum CompileOptions
{
  CASELESS        = 0x0001,
  MULTILINE       = 0x0002,
  DOTALL          = 0x0004,
  EXTENDED        = 0x0008,
  ANCHORED        = 0x0010,
  DOLLAR_ENDONLY  = 0x0020,
  EXTRA           = 0x0040,
  UNGREEDY        = 0x0200
};

inline CompileOptions operator|(CompileOptions lhs, CompileOptions rhs)
  { return static_cast<CompileOptions>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs)); }

inline CompileOptions operator&(CompileOptions lhs, CompileOptions rhs)
  { return static_cast<CompileOptions>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs)); }

inline CompileOptions operator^(CompileOptions lhs, CompileOptions rhs)
  { return static_cast<CompileOptions>(static_cast<unsigned>(lhs) ^ static_cast<unsigned>(rhs)); }

inline CompileOptions operator~(CompileOptions flags)
  { return static_cast<CompileOptions>(~static_cast<unsigned>(flags)); }

inline CompileOptions& operator|=(CompileOptions& lhs, CompileOptions rhs)
  { return (lhs = static_cast<CompileOptions>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs))); }

inline CompileOptions& operator&=(CompileOptions& lhs, CompileOptions rhs)
  { return (lhs = static_cast<CompileOptions>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs))); }

inline CompileOptions& operator^=(CompileOptions& lhs, CompileOptions rhs)
  { return (lhs = static_cast<CompileOptions>(static_cast<unsigned>(lhs) ^ static_cast<unsigned>(rhs))); }


/*
 * The numeric values are copied from pcre.h.  This is quite safe
 * because they cannot be changed without breaking ABI.
 */
enum MatchOptions
{
  NOT_BOL   = 0x0080,
  NOT_EOL   = 0x0100,
  NOT_EMPTY = 0x0400
};

inline MatchOptions operator|(MatchOptions lhs, MatchOptions rhs)
  { return static_cast<MatchOptions>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs)); }

inline MatchOptions operator&(MatchOptions lhs, MatchOptions rhs)
  { return static_cast<MatchOptions>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs)); }

inline MatchOptions operator^(MatchOptions lhs, MatchOptions rhs)
  { return static_cast<MatchOptions>(static_cast<unsigned>(lhs) ^ static_cast<unsigned>(rhs)); }

inline MatchOptions operator~(MatchOptions flags)
  { return static_cast<MatchOptions>(~static_cast<unsigned>(flags)); }

inline MatchOptions& operator|=(MatchOptions& lhs, MatchOptions rhs)
  { return (lhs = static_cast<MatchOptions>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs))); }

inline MatchOptions& operator&=(MatchOptions& lhs, MatchOptions rhs)
  { return (lhs = static_cast<MatchOptions>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs))); }

inline MatchOptions& operator^=(MatchOptions& lhs, MatchOptions rhs)
  { return (lhs = static_cast<MatchOptions>(static_cast<unsigned>(lhs) ^ static_cast<unsigned>(rhs))); }


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


/*
 * Pcre::Pattern represents a compiled regular expression.
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
  void* pcre_;
  int*  ovector_;
  int   ovecsize_;

  Pattern(const Pattern&);
  Pattern& operator=(const Pattern&);
};

} // namespace Pcre

#endif /* REGEXXER_PCRESHELL_H_INCLUDED */
