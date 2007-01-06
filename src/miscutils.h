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

} // namespace Util

#endif /* REGEXXER_MISCUTILS_H_INCLUDED */
