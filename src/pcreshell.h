
#ifndef REGEXXER_PCRESHELL_H_INCLUDED
#define REGEXXER_PCRESHELL_H_INCLUDED

#include <utility>
#include <glibmm/ustring.h>
#include <pcre.h>


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

  Glib::ustring what()   const { return message_; }
  int           offset() const { return offset_;  }

private:
  Glib::ustring message_;
  int           offset_;
};


class Pattern
{
public:
  explicit Pattern(const Glib::ustring& regex, CompileOptions options = CompileOptions(0));
  ~Pattern();

  int match(const Glib::ustring& subject, int offset = 0, MatchOptions options = MatchOptions(0));

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

#endif

