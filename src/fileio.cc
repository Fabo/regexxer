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

#include "fileio.h"
#include "filebuffer.h"
#include "stringutils.h"

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>

#include <glib.h>
#include <glibmm.h>


namespace
{

using Regexxer::FileBuffer;

enum { BUFSIZE = 4096 };

const char fallback_encoding[] = "ISO-8859-1";


Glib::RefPtr<FileBuffer> load_stream(std::istream& input)
{
  const Glib::RefPtr<FileBuffer> text_buffer = FileBuffer::create();

  const Glib::ScopedPtr<char> inbuf (g_new(char, BUFSIZE));
  size_t length_incomplete = 0;

  while(input)
  {
    input.read(inbuf.get() + length_incomplete, BUFSIZE - length_incomplete);
    const size_t n_read = input.gcount();

    // For now, let's assume that any invalid UTF-8 in the input is
    // just an incomplete character at the end of the buffer.
    const char* start_incomplete = 0;
    g_utf8_validate(inbuf.get(), length_incomplete + n_read, &start_incomplete);
    length_incomplete = (inbuf.get() + length_incomplete + n_read) - start_incomplete;

    // If the remaining buffer space after the valid area is >= 6 bytes (the
    // maximum length of a single UTF-8 character), it can't in fact be just
    // an incomplete sequence.  Report failure.
    if(length_incomplete >= 6)
      return Glib::RefPtr<FileBuffer>();

    // Insert all the valid stuff into the text buffer.
    text_buffer->insert(text_buffer->end(), inbuf.get(), start_incomplete);

    // Move the trailing invalid sequence to the front.
    std::memcpy(inbuf.get(), start_incomplete, length_incomplete);
  }

  // At the end of the file there shouldn't be any invalid sequence left.
  if(length_incomplete > 0)
    return Glib::RefPtr<FileBuffer>();

  return text_buffer;
}

Glib::RefPtr<FileBuffer> load_convert_stream(std::istream& input, Glib::IConv& iconv)
{
  const Glib::RefPtr<FileBuffer> text_buffer = FileBuffer::create();

  const Glib::ScopedPtr<char> inbuf   (g_new(char, BUFSIZE));
  const Glib::ScopedPtr<char> convbuf (g_new(char, BUFSIZE));

  gsize inleft = 0;

  while(input)
  {
    // inleft might be > 0 if there was an incomplete
    // multibyte sequence at the end of inbuf.
    input.read(inbuf.get() + inleft, BUFSIZE - inleft);
    inleft += input.gcount();

    char* inpos    = inbuf.get();
    char* convpos  = convbuf.get();
    gsize convleft = BUFSIZE;

    while(iconv.iconv(&inpos, &inleft, &convpos, &convleft) == static_cast<size_t>(-1))
    {
      const int err_no = errno;

      if(err_no == E2BIG)
      {
        // The last character written to convbuf might be incomplete.
        char *const start_incomplete = g_utf8_find_prev_char(convbuf.get(), convpos);
        g_assert(start_incomplete != 0);

        // Gtk::TextBuffer doesn't like '\0' characters.
        if(Util::contains_null(convbuf.get(), start_incomplete))
          return Glib::RefPtr<FileBuffer>();

        // Append what we have so far.
        text_buffer->insert(text_buffer->end(), convbuf.get(), start_incomplete);

        // Move the trailing incomplete sequence to the front.
        const size_t length_incomplete = convpos - start_incomplete;
        std::memcpy(convbuf.get(), start_incomplete, length_incomplete);

        convpos  = convbuf.get() + length_incomplete;
        convleft = BUFSIZE - length_incomplete;
      }
      else if(err_no == EINVAL)
      {
        // Move the trailing incomplete sequence to the front.
        std::memcpy(inbuf.get(), inpos, inleft);
        break;
      }
      else
      {
        if(err_no != EILSEQ)
          g_error("unexpected IConv error: %s", g_strerror(err_no));

        return Glib::RefPtr<FileBuffer>();
      }
    }

    // Gtk::TextBuffer doesn't like '\0' characters.
    if(Util::contains_null(convbuf.get(), convpos))
      return Glib::RefPtr<FileBuffer>();

    // Append what we have so far.
    text_buffer->insert(text_buffer->end(), convbuf.get(), convpos);
  }

  // At the end of the file there shouldn't be any data left.
  if(inleft > 0)
    return Glib::RefPtr<FileBuffer>();

  return text_buffer;
}

void save_stream(std::ostream& output, const Glib::RefPtr<FileBuffer>& buffer)
{
  FileBuffer::iterator start = buffer->begin();
  FileBuffer::iterator stop  = start;

  for(; start; start = stop)
  {
    stop.forward_chars(BUFSIZE); // inaccurate, but doesn't matter
    const Glib::ustring chunk (buffer->get_text(start, stop));

    output.write(chunk.data(), chunk.bytes());
  }
}

void save_convert_stream(std::ostream& output, Glib::IConv& iconv,
                         const Glib::RefPtr<FileBuffer>& buffer)
{
  const Glib::ScopedPtr<char> convbuf (g_new(char, BUFSIZE));

  FileBuffer::iterator start = buffer->begin();
  FileBuffer::iterator stop  = start;

  for(; start; start = stop)
  {
    stop.forward_chars(BUFSIZE); // inaccurate, but doesn't matter
    const Glib::ustring chunk (buffer->get_text(start, stop));

    char* inpos    = const_cast<char*>(chunk.data());
    gsize inleft   = chunk.bytes();
    char* convpos  = convbuf.get();
    gsize convleft = BUFSIZE;

    while(iconv.iconv(&inpos, &inleft, &convpos, &convleft) == static_cast<size_t>(-1))
    {
      const int err_no = errno;

      if(err_no != E2BIG)
        g_error("unexpected IConv error: %s", g_strerror(err_no));

      output.write(convbuf.get(), BUFSIZE - convleft);

      convpos  = convbuf.get();
      convleft = BUFSIZE;
    }

    output.write(convbuf.get(), BUFSIZE - convleft);
  }
}

} // anonymous namespace


namespace Regexxer
{

/**** Regexxer::FileInfo ***************************************************/

FileInfo::FileInfo(const std::string& fullname_)
:
  fullname    (fullname_),
  load_failed (false)
{}

FileInfo::~FileInfo()
{}


/**** Regexxer -- file I/O functions ***************************************/

void load_file(const FileInfoPtr& fileinfo)
{
  std::ifstream input_stream;
  input_stream.exceptions(std::ios_base::badbit);
  input_stream.open(fileinfo->fullname.c_str(), std::ios::in | std::ios::binary);

  std::string encoding = "UTF-8";
  Glib::RefPtr<FileBuffer> buffer = load_stream(input_stream);

  if(!buffer)
  {
    if(!Glib::get_charset(encoding)) // locale charset is _not_ UTF-8
    {
      input_stream.clear();
      input_stream.seekg(0);
      Glib::IConv iconv ("UTF-8", encoding);
      buffer = load_convert_stream(input_stream, iconv);
    }
    if(!buffer && !Util::encodings_equal(encoding, fallback_encoding))
    {
      input_stream.clear();
      input_stream.seekg(0);
      encoding = fallback_encoding;
      Glib::IConv iconv ("UTF-8", encoding);
      buffer = load_convert_stream(input_stream, iconv);
    }
    if(!buffer)
    {
      fileinfo->load_failed = true;
      std::cerr << "Couldn't convert `"
                << Util::filename_to_utf8_fallback(fileinfo->fullname)
                << "' to UTF-8.\n";
      return;
    }
  }

  buffer->set_modified(false);
  fileinfo->load_failed = false;
  fileinfo->encoding    = encoding;
  fileinfo->buffer      = buffer;
}

void save_file(const FileInfoPtr& fileinfo)
{
  std::ofstream output_stream;
  output_stream.exceptions(std::ios_base::badbit | std::ios::failbit);
  output_stream.open(fileinfo->fullname.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);

  if(Util::encodings_equal(fileinfo->encoding, "UTF-8"))
  {
    save_stream(output_stream, fileinfo->buffer);
  }
  else
  {
    Glib::IConv iconv (fileinfo->encoding, "UTF-8");
    save_convert_stream(output_stream, iconv, fileinfo->buffer);
  }

  output_stream.close(); // might throw

  fileinfo->buffer->set_modified(false);
}

} // namespace Regexxer

