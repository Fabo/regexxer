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

#include "stringutils.h"

#include <algorithm>
#include <utility>
#include <vector>
#include <glib/gmessages.h>
#include <glibmm.h>


namespace
{

typedef std::pair<int,char> ModPos;

struct IsSignificantEncodingChar
{
  inline bool operator()(char c) const;
};

inline char ascii_toupper     (char c);
inline bool ascii_isodigit    (char c);
std::string apply_modifiers   (const std::string& subject, const std::vector<ModPos>& modifiers);
std::string parse_control_char(std::string::const_iterator& p, std::string::const_iterator pend);
std::string parse_hex_unichar (std::string::const_iterator& p, std::string::const_iterator pend);
std::string parse_oct_unichar (std::string::const_iterator& p, std::string::const_iterator pend);


inline
bool IsSignificantEncodingChar::operator()(char c) const
{
  switch(c)
  {
    case '-': case '_': case '.': case ' ':
      return false;
  }

  return true;
}

inline
char ascii_toupper(char c)
{
  return (Glib::Ascii::islower(c)) ? (c & '\xDF') : c;
}

inline
bool ascii_isodigit(char c)
{
  return (c >= '0' && c <= '7');
}

std::string apply_modifiers(const std::string& subject, const std::vector<ModPos>& modifiers)
{
  std::string result;
  result.reserve(subject.size());

  typedef std::string::size_type size_type;
  size_type idx = 0;

  std::vector<ModPos>::const_iterator       p    = modifiers.begin();
  const std::vector<ModPos>::const_iterator pend = modifiers.end();

  while(p != pend)
  {
    const size_type start = p->first;
    result.append(subject, idx, start - idx);
    idx = start;

    const char mod = p->second;
    ++p;

    switch(mod)
    {
      case 'L': case 'U':
      {
        while(p != pend && Glib::Ascii::islower(p->second)) { ++p; }

        const size_type stop = (p == pend) ? subject.size() : p->first;
        const Glib::ustring str (subject.begin() + start, subject.begin() + stop);

        result.append((mod == 'L') ? str.lowercase() : str.uppercase());
        idx = stop;
        break;
      }
      case 'l': case 'u':
      {
        if(start < subject.size())
        {
          Glib::ustring::const_iterator cpos (subject.begin() + start);
          gunichar uc = *cpos++;

          uc = (mod == 'l') ? Glib::Unicode::tolower(uc) : Glib::Unicode::totitle(uc);
          const Glib::ustring str (1, uc);

          result.append(str.raw());
          idx = cpos.base() - subject.begin();
        }
        break;
      }
      case 'E':
      {
        break;
      }
      default:
      {
        g_assert_not_reached();
        break;
      }
    }
  }

  result.append(subject.begin() + idx, subject.end());

  return result;
}

std::string parse_control_char(std::string::const_iterator& p, std::string::const_iterator pend)
{
  const std::string::const_iterator pnext = p + 1;

  if(pnext != pend && (*pnext & '\x80') == 0)
  {
    p = pnext;

    char c = ascii_toupper(*pnext);
    c ^= '\x40'; // flip bit 6

    return (c != 0) ? std::string(1, c) : std::string();
  }

  return std::string("c");
}

std::string parse_hex_unichar(std::string::const_iterator& p, std::string::const_iterator pend)
{
  using namespace Glib;

  std::string::const_iterator pstart = p + 1;

  if(pstart != pend)
  {
    if(*pstart == '{')
    {
      const std::string::const_iterator pstop = std::find(++pstart, pend, '}');

      if(pstop != pend)
      {
        p = pstop;
        gunichar uc = 0;

        for(; pstart != pstop; ++pstart)
        {
          if(!Ascii::isxdigit(*pstart))
            return std::string();

          uc *= 0x10;
          uc += Ascii::xdigit_value(*pstart);
        }

        if(uc == 0 || !Unicode::validate(uc))
          return std::string();

        return ustring(1, uc).raw();
      }
    }
    else if(pstart + 1 != pend && Ascii::isxdigit(pstart[0]) && Ascii::isxdigit(pstart[1]))
    {
      p = pstart + 1;
      gunichar uc = 0x10 * Ascii::xdigit_value(pstart[0]) + Ascii::xdigit_value(pstart[1]);

      if(uc == 0 || !Unicode::validate(uc))
        return std::string();

      return ustring(1, uc).raw();
    }
  }

  return std::string("x");
}

std::string parse_oct_unichar(std::string::const_iterator& p, std::string::const_iterator pend)
{
  gunichar uc = 0;
  std::string::const_iterator pnum = p;

  for(; pnum != pend && (pnum - p) < 3; ++pnum)
  {
    if(!ascii_isodigit(*pnum))
      break;

    uc *= 010;
    uc += Glib::Ascii::digit_value(*pnum);
  }

  if(pnum > p)
  {
    p = pnum - 1;

    if(uc != 0 && Glib::Unicode::validate(uc))
      return Glib::ustring(1, uc).raw();
    else
      return std::string();
  }

  return std::string(1, *p);
}

} // anonymous namespace


/* Test lhs and rhs for equality while ignoring case
 * and several separation characters used in encoding names.
 */
bool Util::encodings_equal(const std::string& lhs, const std::string& rhs)
{
  typedef std::string::const_iterator Iterator;

  Iterator       lhs_pos = lhs.begin();
  Iterator       rhs_pos = rhs.begin();
  const Iterator lhs_end = lhs.end();
  const Iterator rhs_end = rhs.end();

  for(;;)
  {
    lhs_pos = std::find_if(lhs_pos, lhs_end, IsSignificantEncodingChar());
    rhs_pos = std::find_if(rhs_pos, rhs_end, IsSignificantEncodingChar());

    if(lhs_pos == lhs_end || rhs_pos == rhs_end)
      break;

    if(ascii_toupper(*lhs_pos) != ascii_toupper(*rhs_pos))
      return false;

    ++lhs_pos;
    ++rhs_pos;
  }

  return (lhs_pos == lhs_end && rhs_pos == rhs_end);
}

bool Util::contains_null(const char* pbegin, const char* pend)
{
  return (std::find(pbegin, pend, '\0') != pend);
}

Glib::ustring Util::shell_pattern_to_regex(const Glib::ustring& pattern)
{
  std::string result = "\\A";

  std::string::const_iterator       p    = pattern.raw().begin();
  const std::string::const_iterator pend = pattern.raw().end();

  bool in_cclass   = false;
  int  brace_level = 0;

  for(; p != pend; ++p)
  {
    if(!in_cclass)
    {
      switch(*p)
      {
        case '*':
          result += ".*";
          break;

        case '?':
          result += '.';
          break;

        case '[':
          result += '[';
          in_cclass = true;
          break;

        case '{':
          result += "(?:";
          ++brace_level;
          break;

        case '}':
          result += ')';
          --brace_level;
          break;

        case ',':
          result += (brace_level > 0) ? '|' : ',';
          break;

        case ']': case '^': case '$': case '.': case '+':
        case '(': case ')': case '|': case '\\':
          result += '\\';
          // fallthrough

        default:
          result += *p;
          break;
      }
    }
    else // in_cclass == true
    {
      switch(*p)
      {
        case ']':
          result += ']';
          if(!((p[-1] == '[') || (p[-1] == '!' && p[-2] == '[')))
            in_cclass = false;
          break;

        case '!':
          result += (p[-1] == '[') ? '^' : '!';
          break;

        case '\\':
          result += "\\\\";
          break;

        default:
          result += *p;
          break;
      }
    }
  }

  result += "\\z";

  return result;
}

std::string Util::substitute_references(const std::string&   substitution,
                                        const std::string&   subject,
                                        const CaptureVector& captures)
{
  std::string result;
  result.reserve(2 * std::max(substitution.size(), subject.size()));

  std::vector<ModPos> modifiers;

  std::string::const_iterator       p    = substitution.begin();
  const std::string::const_iterator pend = substitution.end();

  for(; p != pend; ++p)
  {
    if(*p == '\\')
    {
      if(++p == pend)
      {
        result += '\\';
        break;
      }
      else switch(*p)
      {
        case 'L': case 'U': case 'l': case 'u': case 'E':
          modifiers.push_back(ModPos(result.size(), *p));
          break;

        case 'a':
          result += '\a';
          break;

        case 'e':
          result += '\x1B';
          break;

        case 'f':
          result += '\f';
          break;

        case 'n':
          result += '\n';
          break;

        case 'r':
          result += '\r';
          break;

        case 't':
          result += '\t';
          break;

        case 'c':
          result += parse_control_char(p, pend);
          break;

        case 'x':
          result += parse_hex_unichar(p, pend);
          break;

        default:
          if(ascii_isodigit(*p))
            result += parse_oct_unichar(p, pend);
          else
            result += *p;
          break;
      }
    }
    else if(*p == '$')
    {
      if(++p == pend)
      {
        result += '$';
        break;
      }

      std::pair<int,int> bounds;

      if(Glib::Ascii::isdigit(*p))
      {
        const unsigned index = Glib::Ascii::digit_value(*p);

        if(index < captures.size())
          bounds = captures[index];
        else
          continue;
      }
      else switch(*p)
      {
        case '+':
          if(captures.size() > 1)
            bounds = captures.back();
          break;

        case '&':
          bounds = captures.front();
          break;

        case '`':
          bounds.first  = 0;
          bounds.second = captures.front().first;
          break;

        case '\'':
          bounds.first  = captures.front().second;
          bounds.second = subject.size();
          break;

        default:
          result += '$';
          result += *p;
          continue;
      }

      if(bounds.first >= 0 && bounds.second > bounds.first)
      {
        const std::string::const_iterator begin = subject.begin();
        result.append(begin + bounds.first, begin + bounds.second);
      }
    }
    else // *p != '\\' && *p != '$'
    {
      result += *p;
    }
  }

  if(!modifiers.empty())
    result = apply_modifiers(result, modifiers);

  return result;
}

Glib::ustring Util::filename_to_utf8_fallback(const std::string& filename)
{
  try
  {
    return Glib::filename_to_utf8(filename);
  }
  catch(const Glib::ConvertError& error)
  {
    if(error.code() != Glib::ConvertError::ILLEGAL_SEQUENCE)
      throw;
  }

  const Glib::ustring filename_utf8 (Glib::locale_to_utf8(filename));

  g_warning("The filename encoding of `%s' is not UTF-8 but G_BROKEN_FILENAMES is unset. "
            "Falling back to locale encoding for backward compatibility, but you should "
            "either set the environment variable G_BROKEN_FILENAMES=1 or convert all your "
            "filenames to UTF-8 encoding, as it should be.", filename_utf8.c_str());

  return filename_utf8;
}

Glib::ustring Util::convert_to_ascii(const std::string& str)
{
  std::string result (str);

  std::string::iterator p    = result.begin();
  std::string::iterator pend = result.end();

  for(; p != pend; ++p)
  {
    if((*p & '\x80') != 0)
      *p = '?';
  }

  return result;
}

Glib::ustring Util::transform_pathname(const Glib::ustring& path, bool shorten)
{
  using namespace Glib;

  static const ustring homedir (filename_to_utf8(get_home_dir()));
  static const ustring::size_type homedir_length = homedir.length();

  if(shorten)
  {
    if(path.length() >= homedir_length && path.compare(0, homedir_length, homedir) == 0)
    {
      ustring result ("~");
      result.append(path, homedir_length, ustring::npos);
      return result;
    }
  }
  else
  {
    if(!path.empty() && *path.begin() == '~')
    {
      ustring result (homedir);
      result.append(path, 1, ustring::npos);
      return result;
    }
  }

  return path;
}

