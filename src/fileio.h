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

#ifndef REGEXXER_FILEIO_H_INCLUDED
#define REGEXXER_FILEIO_H_INCLUDED

#include "sharedptr.h"
#include <string>
#include <glibmm/refptr.h>


namespace Regexxer
{

class FileBuffer;

struct FileInfo : public Util::SharedObject
{
  std::string               fullname;
  std::string               encoding;
  Glib::RefPtr<FileBuffer>  buffer;
  bool                      load_failed;

  explicit FileInfo(const std::string& fullname_);
  ~FileInfo();
};

typedef Util::SharedPtr<FileInfo> FileInfoPtr;

void load_file(const FileInfoPtr& fileinfo, const std::string& fallback_encoding);
void save_file(const FileInfoPtr& fileinfo);

} // namespace Regexxer

#endif /* REGEXXER_FILEIO_H_INCLUDED */

