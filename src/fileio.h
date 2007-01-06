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

#ifndef REGEXXER_FILEIO_H_INCLUDED
#define REGEXXER_FILEIO_H_INCLUDED

#include "sharedptr.h"
#include <string>
#include <glibmm/refptr.h>


namespace Regexxer
{

class FileBuffer;

class FileInfoBase : public Util::SharedObject
{
public:
  virtual ~FileInfoBase() = 0;
};

struct DirInfo : public FileInfoBase
{
  int file_count;
  int modified_count;

  DirInfo();
  virtual ~DirInfo();
};

struct FileInfo : public FileInfoBase
{
  std::string               fullname;
  std::string               encoding;
  Glib::RefPtr<FileBuffer>  buffer;
  bool                      load_failed;

  explicit FileInfo(const std::string& fullname_);
  virtual ~FileInfo();
};

typedef Util::SharedPtr<FileInfoBase> FileInfoBasePtr;
typedef Util::SharedPtr<DirInfo>      DirInfoPtr;
typedef Util::SharedPtr<FileInfo>     FileInfoPtr;

class ErrorBinaryFile {}; // exception type

void load_file(const FileInfoPtr& fileinfo, const std::string& fallback_encoding);
void save_file(const FileInfoPtr& fileinfo);

} // namespace Regexxer

#endif /* REGEXXER_FILEIO_H_INCLUDED */
