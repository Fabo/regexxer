
#ifndef REGEXXER_STRINGUTILS_H_INCLUDED
#define REGEXXER_STRINGUTILS_H_INCLUDED

#include <string>
#include <vector>
#include <glibmm/ustring.h>


namespace Util
{

typedef std::vector< std::pair<int,int> > CaptureVector;

bool encodings_equal(const std::string& lhs, const std::string& rhs);
bool contains_null(const char* pbegin, const char* pend);
Glib::ustring shell_pattern_to_regex(const Glib::ustring& pattern);

std::string substitute_references(const std::string&   substitution,
                                  const std::string&   subject,
                                  const CaptureVector& captures);

Glib::ustring transform_pathname(const Glib::ustring& path, bool shorten);

inline Glib::ustring shorten_pathname(const Glib::ustring& path)
  { return transform_pathname(path, true); }

inline Glib::ustring expand_pathname(const Glib::ustring& path)
  { return transform_pathname(path, false); }

} // namespace Util

#endif

