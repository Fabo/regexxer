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

#ifndef REGEXXER_MISCUTILS_H_INCLUDED
#define REGEXXER_MISCUTILS_H_INCLUDED


namespace Util
{

template <class T>
class ScopedArray
{
private:
  T* array_;

  ScopedArray(const ScopedArray<T>&);
  ScopedArray<T>& operator=(const ScopedArray<T>&);

public:
  explicit ScopedArray(T* array) : array_ (array) {}
  ~ScopedArray() { delete[] array_; }

  T* get() const { return array_; }
};


/* next() and prior(): Idea shamelessly stolen from boost.
 */
template <class Iterator>
inline Iterator next(Iterator pos) { return ++pos; }

template <class Iterator>
inline Iterator prior(Iterator pos) { return --pos; }

/* Return true if the GTK+ version is GTK_MAJOR_VERSION.minor.micro
 * or higher.  There is no parameter for the major version since using
 * a different major release would require (at least) recompiling the
 * program anyway.
 */
bool gtk_version_at_least(unsigned int minor, unsigned int micro);

} // namespace Util

#endif /* REGEXXER_MISCUTILS_H_INCLUDED */

