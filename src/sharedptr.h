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

#ifndef REGEXXER_SHAREDPTR_H_INCLUDED
#define REGEXXER_SHAREDPTR_H_INCLUDED

#include <algorithm>


namespace Util
{

template <class> class SharedPtr;


/* Common base class of objects managed by SharedPtr<>.
 */
class SharedObject
{
protected:
  SharedObject(); // initial reference count is 0
  ~SharedObject();

private:
  mutable int refcount_;

  SharedObject(const SharedObject&);
  SharedObject& operator=(const SharedObject&);

  template <class> friend class SharedPtr;
};


/* Intrusive smart pointer implementation.  It requires the managed types
 * to be derived from class SharedObject, in order to do reference counting
 * as efficient as possible.
 */
template <class T>
class SharedPtr
{
public:
  inline SharedPtr();
  inline ~SharedPtr();

  explicit inline SharedPtr(T* ptr); // obtains reference

  inline SharedPtr(const SharedPtr<T>& other);
  inline SharedPtr<T>& operator=(const SharedPtr<T>& other);

  template <class U> inline SharedPtr(const SharedPtr<U>& other);
  template <class U> inline SharedPtr<T>& operator=(const SharedPtr<U>& other);

  inline void reset(T* ptr = 0); // obtains reference
  inline T*   get() const;

  inline T* operator->() const;
  inline T& operator*()  const;

  inline operator const void*() const;

private:
  T* ptr_;
};


template <class T> inline
SharedPtr<T>::SharedPtr()
:
  ptr_ (0)
{}

template <class T> inline
SharedPtr<T>::~SharedPtr()
{
  if(ptr_ && --ptr_->refcount_ <= 0)
    delete ptr_;
}

template <class T> inline
SharedPtr<T>::SharedPtr(T* ptr)
:
  ptr_ (ptr)
{
  if(ptr_)
    ++ptr_->refcount_;
}

// Note that reset() and get() are defined here and not in declaration order
// on purpose -- defining them before they're first used allows for maximum
// inlining.

template <class T> inline
void SharedPtr<T>::reset(T* ptr)
{
  // Note that SharedPtr<>::reset() obtains a reference.
  SharedPtr<T> temp (ptr);
  std::swap(ptr_, temp.ptr_);
}

template <class T> inline
T* SharedPtr<T>::get() const
{
  return ptr_;
}

template <class T> inline
SharedPtr<T>::SharedPtr(const SharedPtr<T>& other)
:
  ptr_ (other.ptr_)
{
  if(ptr_)
    ++ptr_->refcount_;
}

template <class T> inline
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr<T>& other)
{
  this->reset(other.ptr_);
  return *this;
}

template <class T>
  template <class U>
inline
SharedPtr<T>::SharedPtr(const SharedPtr<U>& other)
:
  ptr_ (other.get())
{
  if(ptr_)
    ++ptr_->refcount_;
}

template <class T>
  template <class U>
inline
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr<U>& other)
{
  this->reset(other.get());
  return *this;
}

template <class T> inline
T* SharedPtr<T>::operator->() const
{
  return ptr_;
}

template <class T> inline
T& SharedPtr<T>::operator*() const
{
  return *ptr_;
}

template <class T> inline
SharedPtr<T>::operator const void*() const
{
  return ptr_;
}

} // namespace Util

#endif

