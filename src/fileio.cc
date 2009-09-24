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

#include "fileio.h"
#include "filebuffer.h"
#include "miscutils.h"
#include "stringutils.h"

#include <glib.h>
#include <glibmm.h>
#include <giomm.h>
#include <gtksourceviewmm.h> 
#include <cstring>

namespace Gsv=gtksourceview;

namespace
{

using Regexxer::FileBuffer;

enum { BUFSIZE = 4096 };

static
Glib::RefPtr<FileBuffer> load_iochannel(const Glib::RefPtr<Glib::IOChannel>& input)
{
  const Glib::RefPtr<FileBuffer> text_buffer = FileBuffer::create();
  FileBuffer::iterator text_end = text_buffer->end();

  const Util::ScopedArray<char> inbuf (new char[BUFSIZE]);
  gsize bytes_read = 0;

  text_buffer->begin_not_undoable_action();
  while (input->read(inbuf.get(), BUFSIZE, bytes_read) == Glib::IO_STATUS_NORMAL)
  {
    if (std::memchr(inbuf.get(), '\0', bytes_read)) // binary file?
      throw Regexxer::ErrorBinaryFile();

    text_end = text_buffer->insert(text_end, inbuf.get(), inbuf.get() + bytes_read);
  }
  text_buffer->end_not_undoable_action();

  g_assert(bytes_read == 0);

  return text_buffer;
}

static
void save_iochannel(const Glib::RefPtr<Glib::IOChannel>& output, const Glib::RefPtr<FileBuffer>& buffer)
{
  FileBuffer::iterator stop = buffer->begin();

  for (FileBuffer::iterator start = stop; start; start = stop)
  {
    stop.forward_chars(BUFSIZE); // inaccurate, but doesn't matter
    const Glib::ustring chunk = buffer->get_slice(start, stop);

    gsize bytes_written = 0;
    const Glib::IOStatus status = output->write(chunk.data(), chunk.bytes(), bytes_written);

    // These conditions really must hold true at this point
    // since any error should have caused an exception.
    g_assert(status == Glib::IO_STATUS_NORMAL);
    g_assert(bytes_written == chunk.bytes());
  }
}

static
Glib::RefPtr<FileBuffer> load_try_encoding(const std::string& filename, const std::string& encoding)
{
  const Glib::RefPtr<Glib::IOChannel> channel = Glib::IOChannel::create_from_file(filename, "r");

  channel->set_buffer_size(BUFSIZE);

  try
  {
    channel->set_encoding(encoding);
    return load_iochannel(channel);
  }
  catch (const Glib::ConvertError&)
  {}

  return Glib::RefPtr<FileBuffer>();
}

} // anonymous namespace


namespace Regexxer
{

/**** Regexxer::FileInfoBase ***********************************************/

FileInfoBase::~FileInfoBase()
{}


/**** Regexxer::DirInfo ****************************************************/

DirInfo::DirInfo()
:
  file_count     (0),
  modified_count (0)
{}

DirInfo::~DirInfo()
{}


/**** Regexxer::FileInfo ***************************************************/

FileInfo::FileInfo(const std::string& fullname_)
:
  fullname    (fullname_),
  load_failed (false)
{}

FileInfo::~FileInfo()
{}


/**** Regexxer -- file I/O functions ***************************************/

void load_file(const FileInfoPtr& fileinfo, const std::string& fallback_encoding)
{
  fileinfo->load_failed = true;

  std::string encoding = "UTF-8";
  Glib::RefPtr<FileBuffer> buffer = load_try_encoding(fileinfo->fullname, encoding);

  if (!buffer && !Glib::get_charset(encoding)) // locale charset is _not_ UTF-8
  {
    buffer = load_try_encoding(fileinfo->fullname, encoding);
  }

  if (!buffer && !Util::encodings_equal(encoding, fallback_encoding))
  {
    encoding = fallback_encoding;
    buffer = load_try_encoding(fileinfo->fullname, encoding);
  }

  if (!buffer)
    throw ErrorBinaryFile();

  Glib::RefPtr<Gsv::SourceLanguageManager> language_manager = Gsv::SourceLanguageManager::create();
  
  bool uncertain = false;
  std::string content_type = Gio::content_type_guess(fileinfo->fullname, buffer->get_text(), uncertain);

  buffer->set_highlight_syntax(true);
  buffer->set_language(language_manager->guess_language(fileinfo->fullname, content_type));
  buffer->set_modified(false);

  fileinfo->encoding    = encoding;
  fileinfo->buffer      = buffer;
  fileinfo->load_failed = false;
}

void save_file(const FileInfoPtr& fileinfo)
{
  const Glib::RefPtr<Glib::IOChannel> channel =
      Glib::IOChannel::create_from_file(fileinfo->fullname, "w");

  channel->set_buffer_size(BUFSIZE);
  channel->set_encoding(fileinfo->encoding);

  save_iochannel(channel, fileinfo->buffer);

  // Explicitely close() the buffer at this point so that
  // we get an exception if closing the file fails.
  channel->close();

  fileinfo->buffer->set_modified(false);
}

} // namespace Regexxer
