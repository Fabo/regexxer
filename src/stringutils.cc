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

#include <glib.h>
#include <glib-object.h>
#include <glibmm.h>
#include <gdkmm/color.h>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include <config.h>

#if REGEXXER_HAVE_STD_LOCALE
#include <locale>
#endif


namespace
{

typedef std::pair<int,char> ModPos;

class ScopedTypeClass
{
private:
  void* class_;

  ScopedTypeClass(const ScopedTypeClass&);
  ScopedTypeClass& operator=(const ScopedTypeClass&);

public:
  explicit ScopedTypeClass(GType type)
    : class_ (g_type_class_ref(type)) {}

  ~ScopedTypeClass() { g_type_class_unref(class_); }

  void* get() const { return class_; }
};


inline
bool is_significant_encoding_char(char c)
{
  switch (c)
  {
    case ' ': case '-': case '_': case '.': case ':':
      return false;
  }

  return true;
}

inline
unsigned int scale_to_8bit(unsigned int value)
{
  return (value & 0xFF00) >> 8;
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

  int idx = 0;

  const std::vector<ModPos>::const_iterator pend = modifiers.end();
  std::vector<ModPos>::const_iterator       p    = modifiers.begin();

  while (p != pend)
  {
    const int start = p->first;
    result.append(subject, idx, start - idx);
    idx = start;

    const char mod = p->second;
    ++p;

    switch (mod)
    {
      case 'L': case 'U':
      {
        while (p != pend && (p->second == 'l' || p->second == 'u'))
          ++p;

        const int stop = (p == pend) ? subject.size() : p->first;
        const Glib::ustring slice (subject.begin() + start, subject.begin() + stop);
        const Glib::ustring str = (mod == 'L') ? slice.lowercase() : slice.uppercase();

        result.append(str.raw());
        idx = stop;
        break;
      }
      case 'l': case 'u': // TODO: Simplify.  This code is way too complicated.
      {
        if (unsigned(start) < subject.size())
        {
          while (p != pend && p->first == start && p->second != 'L' && p->second != 'U')
            ++p;

          if (p != pend && p->first == start)
          {
            const char submod = p->second;

            do
              ++p;
            while (p != pend && (p->second == 'l' || p->second == 'u'));

            const int stop = (p == pend) ? subject.size() : p->first;
            const Glib::ustring slice (subject.begin() + start, subject.begin() + stop);
            const Glib::ustring str = (submod == 'L') ? slice.lowercase() : slice.uppercase();

            if (!str.empty())
            {
              Glib::ustring::const_iterator cpos = str.begin();
              gunichar uc = *cpos++;
              uc = (mod == 'l') ? Glib::Unicode::tolower(uc) : Glib::Unicode::totitle(uc);

              if (Glib::Unicode::validate(uc))
                result.append(Glib::ustring(1, uc).raw());

              result.append(cpos.base(), str.end().base());
            }
            idx = stop;
          }
          else
          {
            Glib::ustring::const_iterator cpos (subject.begin() + start);
            gunichar uc = *cpos++;
            uc = (mod == 'l') ? Glib::Unicode::tolower(uc) : Glib::Unicode::totitle(uc);

            if (Glib::Unicode::validate(uc))
              result.append(Glib::ustring(1, uc).raw());

            idx = cpos.base() - subject.begin();
          }
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

  result.append(subject, idx, std::string::npos);

  return result;
}

void parse_control_char(std::string::const_iterator& p, std::string::const_iterator pend,
                        std::string& dest)
{
  const std::string::const_iterator pnext = p + 1;

  if (pnext != pend && (*pnext & '\x80') == 0)
  {
    p = pnext;

    // Flip bit 6 of the upcased character.
    const char c = Glib::Ascii::toupper(*pnext) ^ '\x40';

    // TextBuffer can't handle NUL; interpret it as empty string instead.
    if (c != '\0')
      dest += c;
  }
  else
    dest += 'c';
}

void parse_hex_unichar(std::string::const_iterator& p, std::string::const_iterator pend,
                       std::string& dest)
{
  using namespace Glib;

  std::string::const_iterator pstart = p + 1;

  if (pstart != pend)
  {
    if (*pstart == '{')
    {
      const std::string::const_iterator pstop = std::find(++pstart, pend, '}');

      if (pstop != pend)
      {
        p = pstop;
        gunichar uc = 0;

        for (; pstart != pstop; ++pstart)
        {
          if (!Ascii::isxdigit(*pstart))
            return;

          uc *= 0x10;
          uc += Ascii::xdigit_value(*pstart);
        }

        if (uc != 0 && Unicode::validate(uc))
          dest += ustring(1, uc).raw();

        return;
      }
    }
    else if (pstart + 1 != pend && Ascii::isxdigit(pstart[0]) && Ascii::isxdigit(pstart[1]))
    {
      p = pstart + 1;
      const gunichar uc = 0x10 * Ascii::xdigit_value(pstart[0]) + Ascii::xdigit_value(pstart[1]);

      if (uc != 0 && Unicode::validate(uc))
        dest += ustring(1, uc).raw();

      return;
    }
  }

  dest += 'x';
}

void parse_oct_unichar(std::string::const_iterator& p, std::string::const_iterator pend,
                       std::string& dest)
{
  gunichar uc = 0;
  std::string::const_iterator pnum = p;

  for (; pnum != pend && (pnum - p) < 3; ++pnum)
  {
    if (!ascii_isodigit(*pnum))
      break;

    uc *= 010;
    uc += Glib::Ascii::digit_value(*pnum);
  }

  if (pnum > p)
  {
    p = pnum - 1;

    if (uc != 0 && Glib::Unicode::validate(uc))
      dest += Glib::ustring(1, uc).raw();
  }
  else
    dest += *p;
}

/*
 * On entry, p _must_ point to either a digit or a starting bracket '{'.  Also,
 * if p points to '{' the closing bracket '}' is assumed to follow before pend.
 */
int parse_capture_index(std::string::const_iterator& p, std::string::const_iterator pend)
{
  std::string::const_iterator pnum = p;

  if (*pnum == '{' && *++pnum == '}')
  {
    p = pnum;
    return -1;
  }

  int result = 0;

  while (pnum != pend && Glib::Ascii::isdigit(*pnum))
  {
    result *= 10;
    result += Glib::Ascii::digit_value(*pnum++);
  }

  if (*p != '{') // case "$digits": set position to last digit
  {
    p = pnum - 1;
  }
  else if (*pnum == '}') // case "${digits}": set position to '}'
  {
    p = pnum;
  }
  else // case "${invalid}": return -1 but still skip until '}'
  {
    p = std::find(pnum, pend, '}');
    return -1;
  }

  return result;
}

} // anonymous namespace


bool Util::validate_encoding(const std::string& encoding)
{
  // GLib just ignores some characters that aren't used in encoding names,
  // so we have to parse the string for invalid characters ourselves.

  for (std::string::const_iterator p = encoding.begin(); p != encoding.end(); ++p)
  {
    if (!Glib::Ascii::isalnum(*p) && is_significant_encoding_char(*p))
      return false;
  }

  try
  {
    Glib::convert("", "UTF-8", encoding);
  }
  catch (const Glib::ConvertError& error)
  {
    if (error.code() == Glib::ConvertError::NO_CONVERSION)
      return false;
    throw;
  }

  return true;
}

/*
 * Test lhs and rhs for equality while ignoring case
 * and several separation characters used in encoding names.
 */
bool Util::encodings_equal(const std::string& lhs, const std::string& rhs)
{
  typedef std::string::const_iterator Iterator;

  Iterator       lhs_pos = lhs.begin();
  Iterator       rhs_pos = rhs.begin();
  const Iterator lhs_end = lhs.end();
  const Iterator rhs_end = rhs.end();

  for (;;)
  {
    while (lhs_pos != lhs_end && !is_significant_encoding_char(*lhs_pos))
      ++lhs_pos;
    while (rhs_pos != rhs_end && !is_significant_encoding_char(*rhs_pos))
      ++rhs_pos;

    if (lhs_pos == lhs_end || rhs_pos == rhs_end)
      break;

    if (Glib::Ascii::toupper(*lhs_pos) != Glib::Ascii::toupper(*rhs_pos))
      return false;

    ++lhs_pos;
    ++rhs_pos;
  }

  return (lhs_pos == lhs_end && rhs_pos == rhs_end);
}

Glib::ustring Util::shell_pattern_to_regex(const Glib::ustring& pattern)
{
  // Don't use Glib::ustring to accumulate the result since we might append
  // partial UTF-8 characters during processing.  Although this would work with
  // the current Glib::ustring implementation, it's definitely not a good idea.
  std::string result;
  result.reserve(std::max<std::string::size_type>(32, 2 * pattern.bytes()));

  result.append("\\A", 2);

  int  brace_level = 0;
  bool in_cclass   = false;

  const std::string::const_iterator pend = pattern.end().base();
  std::string::const_iterator       p    = pattern.begin().base();

  for (; p != pend; ++p)
  {
    if (*p == '\\')
    {
      // Always escape a single trailing '\' to avoid mangling the "\z"
      // terminator.  Never escape multi-byte or alpha-numeric characters.

      if (p + 1 == pend || Glib::Ascii::ispunct(*++p))
        result += '\\';

      result += *p;
    }
    else if (!in_cclass)
    {
      switch (*p)
      {
        case '*':
          result.append(".*", 2);
          break;

        case '?':
          result += '.';
          break;

        case '[':
          result += '[';
          in_cclass = true;
          break;

        case '{':
          result.append("(?:", 3);
          ++brace_level;
          break;

        case '}':
          result += ')';
          --brace_level;
          break;

        case ',':
          result += (brace_level > 0) ? '|' : ',';
          break;

        case '^': case '$': case '.': case '+': case '(': case ')': case '|':
          result += '\\';
          // fallthrough

        default:
          result += *p;
          break;
      }
    }
    else // in_cclass == true
    {
      // Note that the negative indices below are safe because at least
      // the opening '[' must have been preceding in order to get here.
      switch (*p)
      {
        case ']':
          result += ']';
          in_cclass = ((p[-1] == '[') || ((p[-1] == '!' || p[-1] == '^') && p[-2] == '['));
          break;

        case '!':
          result += (p[-1] == '[') ? '^' : '!';
          break;

        default:
          result += *p;
          break;
      }
    }
  }

  result.append("\\z", 2);

  return result;
}

std::string Util::substitute_references(const std::string&   substitution,
                                        const std::string&   subject,
                                        const CaptureVector& captures)
{
  std::string result;
  result.reserve(2 * std::max(substitution.size(), subject.size()));

  std::vector<ModPos> modifiers;

  const std::string::const_iterator pend = substitution.end();
  std::string::const_iterator       p    = substitution.begin();

  for (; p != pend; ++p)
  {
    if (*p == '\\' && p + 1 != pend)
    {
      switch (*++p)
      {
        case 'L': case 'U': case 'l': case 'u': case 'E':
          modifiers.push_back(ModPos(result.size(), *p));
          break;

        case 'a':
          result += '\a';
          break;

        case 'e':
          result += '\033';
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
          parse_control_char(p, pend, result);
          break;

        case 'x':
          parse_hex_unichar(p, pend, result);
          break;

        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
          parse_oct_unichar(p, pend, result);
          break;

        default:
          result += *p;
          break;
      }
    }
    else if (*p == '$' && p + 1 != pend)
    {
      std::pair<int,int> bounds;

      if (Glib::Ascii::isdigit(*++p) || (*p == '{' && std::find(p, pend, '}') != pend))
      {
        const int index = parse_capture_index(p, pend);

        if (index >= 0 && unsigned(index) < captures.size())
          bounds = captures[index];
        else
          continue;
      }
      else switch (*p)
      {
        case '+':
          if (captures.size() > 1)
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

      if (bounds.first >= 0 && bounds.second > bounds.first)
        result.append(subject, bounds.first, bounds.second - bounds.first);
    }
    else // (*p != '\\' && *p != '$') || (p + 1 == pend)
    {
      result += *p;
    }
  }

  if (!modifiers.empty())
    result = apply_modifiers(result, modifiers);

  return result;
}

Glib::ustring Util::filename_to_utf8_fallback(const std::string& filename)
{
  try
  {
    return Glib::filename_to_utf8(filename);
  }
  catch (const Glib::ConvertError& error)
  {
    if (error.code() != Glib::ConvertError::ILLEGAL_SEQUENCE)
      throw;
  }

  const Glib::ustring filename_utf8 = Util::convert_to_ascii(filename);

  g_warning("The filename `%s' is not valid UTF-8.  To work around that, please "
            "set G_FILENAME_ENCODING to the local encoding used for filenames.  "
            "Alternatively, convert all your legacy filenames to UTF-8 encoding.",
            filename_utf8.c_str());

  return filename_utf8;
}

Glib::ustring Util::convert_to_ascii(const std::string& str)
{
  std::ostringstream output;

#if REGEXXER_HAVE_STD_LOCALE
  output.imbue(std::locale::classic());
#endif

  output.setf(std::ios::oct, std::ios::basefield);

  for (std::string::const_iterator p = str.begin(); p != str.end(); ++p)
  {
    if ((*p & '\x80') == 0)
      output << *p;
    else
      output << '\\' << static_cast<unsigned int>(static_cast<unsigned char>(*p));
  }

  return output.str();
}

Glib::ustring Util::int_to_string(int number)
{
  std::ostringstream output;

#if REGEXXER_HAVE_STD_LOCALE
  try // don't abort if the user-specified locale doesn't exist
  {
    output.imbue(std::locale(""));
  }
  catch (const std::runtime_error& error)
  {
    g_warning("%s", error.what());
  }
#endif

  output << number;

  return Glib::locale_to_utf8(output.str());
}

std::string Util::shorten_pathname(const std::string& path)
{
  const std::string homedir = Glib::get_home_dir();
  const std::string::size_type len = homedir.length();

  if (path.length() >= len
      && (path.length() == len || path[len] == G_DIR_SEPARATOR)
      && path.compare(0, len, homedir) == 0)
  {
    std::string result (1, '~');
    result.append(path, len, std::string::npos);
    return result;
  }

  return path;
}

std::string Util::expand_pathname(const std::string& path)
{
  if (path.length() > 0 && path[0] == '~'
      && (path.length() == 1 || path[1] == G_DIR_SEPARATOR))
  {
    std::string result = Glib::get_home_dir();
    result.append(path, 1, std::string::npos);
    return result;
  }

  return path;
}

Glib::ustring Util::color_to_string(const Gdk::Color& color)
{
  std::ostringstream output;

#if REGEXXER_HAVE_STD_LOCALE
  output.imbue(std::locale::classic());
#endif

  output.setf(std::ios::hex, std::ios::basefield);
  output.setf(std::ios::uppercase);
  output.fill('0');

  output << '#' << std::setw(2) << scale_to_8bit(color.get_red())
                << std::setw(2) << scale_to_8bit(color.get_green())
                << std::setw(2) << scale_to_8bit(color.get_blue());

  return output.str();
}

int Util::enum_from_nick_impl(GType type, const Glib::ustring& nick)
{
  const ScopedTypeClass type_class (type);

  GEnumClass *const enum_class = G_ENUM_CLASS(type_class.get());
  GEnumValue *const enum_value = g_enum_get_value_by_nick(enum_class, nick.c_str());

  g_return_val_if_fail(enum_value != 0, enum_class->minimum);

  return enum_value->value;
}

Glib::ustring Util::enum_to_nick_impl(GType type, int value)
{
  const ScopedTypeClass type_class (type);

  GEnumClass *const enum_class = G_ENUM_CLASS(type_class.get());
  GEnumValue *const enum_value = g_enum_get_value(enum_class, value);

  g_return_val_if_fail(enum_value != 0, "");

  return enum_value->value_nick;
}

