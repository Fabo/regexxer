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

#include "statusline.h"
#include <gtkmm.h>

#include <config.h>

#if REGEXXER_HAVE_STD_LOCALE
#include <locale>
#endif


namespace Regexxer
{

/**** Regexxer::StatusLine *************************************************/

StatusLine::StatusLine()
:
  Gtk::HBox(false, 1)
{
  using namespace Gtk;

  progressbar_ = new ProgressBar();
  pack_start(*manage(progressbar_), PACK_SHRINK);

  Frame *const frame_files = new Frame();
  pack_start(*manage(frame_files), PACK_SHRINK);
  frame_files->set_shadow_type(SHADOW_IN);

  Frame *const frame_matches = new Frame();
  pack_start(*manage(frame_matches), PACK_SHRINK);
  frame_matches->set_shadow_type(SHADOW_IN);

  statusbar_ = new Statusbar();
  pack_start(*manage(statusbar_), PACK_EXPAND_WIDGET);

  label_files_ = new Label("", 0.0, 0.5);
  frame_files->add(*manage(label_files_));
  label_files_->set_padding(2, 0);

  label_matches_ = new Label("", 0.0, 0.5);
  frame_matches->add(*manage(label_matches_));
  label_matches_->set_padding(2, 0);

  label_files_->signal_style_changed().connect(
      SigC::bind(SigC::slot(*this, &StatusLine::on_label_style_changed), label_files_));

  label_matches_->signal_style_changed().connect(
      SigC::bind(SigC::slot(*this, &StatusLine::on_label_style_changed), label_matches_));

#if REGEXXER_HAVE_STD_LOCALE
  stringstream_.imbue(std::locale(""));
#endif

  set_file_count(0);
  set_match_count(0);
}

StatusLine::~StatusLine()
{}

void StatusLine::set_file_count(int file_count)
{
  label_files_->set_text("Files: " + number_to_string(file_count));
}

void StatusLine::set_match_count(long match_count)
{
  label_matches_->set_text("Matches: " + number_to_string(match_count));
}

void StatusLine::pulse()
{
  progressbar_->pulse();
}

void StatusLine::stop_pulse()
{
  progressbar_->set_fraction(0.0);
}

/**** Regexxer::StatusLine -- private **************************************/

Glib::ustring StatusLine::number_to_string(unsigned long number)
{
  stringstream_.str(std::string());
  stringstream_ << number;
  return Glib::locale_to_utf8(stringstream_.str());
}

void StatusLine::on_label_style_changed(const Glib::RefPtr<Gtk::Style>&, Gtk::Label* label)
{
  Glib::ustring text = (label == label_files_) ? "Files: " : "Matches: ";
  text += number_to_string(99999);

  int width = 0, height;
  label->create_pango_layout(text)->get_pixel_size(width, height);

  int pad_x = 0, pad_y;
  label->get_padding(pad_x, pad_y);

  label->set_size_request(width + 2 * pad_x, -1);
}

} // namespace Regexxer

