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
#include <sstream>

#include <config.h>

#if REGEXXER_HAVE_STD_LOCALE
#include <locale>
#endif


namespace Regexxer
{

/**** Regexxer::CounterBox *************************************************/

class CounterBox : public Gtk::Frame
{
public:
  explicit CounterBox(const Glib::ustring& label);
  virtual ~CounterBox();

  void set_index(unsigned long index);
  void set_count(unsigned long count);

protected:
  virtual void on_style_changed(const Glib::RefPtr<Gtk::Style>& previous_style);

private:
  Gtk::Label*         label_index_;
  Gtk::Label*         label_count_;
  unsigned long       index_;
  bool                refreshing_;
  unsigned long       digit_range_;
  std::ostringstream  stringstream_;

  Glib::ustring number_to_string(unsigned long number);
  void recalculate_label_width();
  static void set_width_from_string(Gtk::Label& label, const Glib::ustring& text);

  bool idle_refresh_label_index();
};


CounterBox::CounterBox(const Glib::ustring& label)
:
  label_index_  (0),
  label_count_  (0),
  index_        (0),
  refreshing_   (false),
  digit_range_  (10)
{
  using namespace Gtk;

  set_shadow_type(SHADOW_IN);

  Box *const paddingbox = new HBox(false, 0);
  add(*manage(paddingbox));

  Box *const box = new HBox(false, 0);
  paddingbox->pack_start(*manage(box), PACK_SHRINK, 2);

  box->pack_start(*manage(new Label(label)), PACK_SHRINK);

  label_index_ = new Label("", 1.0, 0.5);
  box->pack_start(*manage(label_index_), PACK_SHRINK);

  box->pack_start(*manage(new Label("/")), PACK_SHRINK, 2);

  label_count_ = new Label("", 0.0, 0.5);
  box->pack_start(*manage(label_count_), PACK_SHRINK);

#if REGEXXER_HAVE_STD_LOCALE
  stringstream_.imbue(std::locale(""));
#endif

  set_index(0);
  set_count(0);

  paddingbox->show_all();
}

CounterBox::~CounterBox()
{}

void CounterBox::set_index(unsigned long index)
{
  // Work around a bug in GTK+ that causes right-aligned labels to be
  // cut off at the end.  The label magically displays correctly if the
  // text is changed in an idle callback.

  index_ = index; // remember value for use in idle callback

  if(!refreshing_)
  {
    refreshing_ = true;

    Glib::signal_idle().connect(
        SigC::slot(*this, &CounterBox::idle_refresh_label_index),
        Glib::PRIORITY_HIGH_IDLE);
  }
}

void CounterBox::set_count(unsigned long count)
{
  unsigned long range = digit_range_;

  while(range <= count)
    range *= 10;

  while(range > 10 && range / 10 > count)
    range /= 10;

  if(range != digit_range_)
  {
    digit_range_ = range;
    recalculate_label_width();
  }

  label_count_->set_text(number_to_string(count));
}

void CounterBox::on_style_changed(const Glib::RefPtr<Gtk::Style>& previous_style)
{
  Gtk::Frame::on_style_changed(previous_style);
  recalculate_label_width();
}

Glib::ustring CounterBox::number_to_string(unsigned long number)
{
  stringstream_.str(std::string());
  stringstream_ << number;
  return Glib::locale_to_utf8(stringstream_.str());
}

void CounterBox::recalculate_label_width()
{
  const Glib::ustring text = number_to_string(digit_range_ - 1);

  set_width_from_string(*label_index_, text);
  set_width_from_string(*label_count_, text);
}

// static
void CounterBox::set_width_from_string(Gtk::Label& label, const Glib::ustring& text)
{
  int width = 0, height = 0, xpad = 0, ypad = 0;

  label.create_pango_layout(text)->get_pixel_size(width, height);
  label.get_padding(xpad, ypad);
  label.set_size_request(width + 2 * xpad, -1);
}

bool CounterBox::idle_refresh_label_index()
{
  label_index_->set_text(number_to_string(index_));

  refreshing_ = false;
  return false;
}


/**** Regexxer::StatusLine *************************************************/

StatusLine::StatusLine()
:
  Gtk::HBox(false, 1),
  progressbar_    (0),
  file_counter_   (0),
  match_counter_  (0),
  statusbar_      (0)
{
  using namespace Gtk;

  progressbar_ = new ProgressBar();
  pack_start(*manage(progressbar_), PACK_SHRINK);

  file_counter_ = new CounterBox("File: ");
  pack_start(*manage(file_counter_), PACK_SHRINK);

  match_counter_ = new CounterBox("Match: ");
  pack_start(*manage(match_counter_), PACK_SHRINK);

  statusbar_ = new Statusbar();
  pack_start(*manage(statusbar_), PACK_EXPAND_WIDGET);

  show_all_children();
}

StatusLine::~StatusLine()
{}

void StatusLine::set_file_index(int file_index)
{
  file_counter_->set_index(file_index);
}

void StatusLine::set_file_count(int file_count)
{
  file_counter_->set_count(file_count);
}

void StatusLine::set_match_index(long match_index)
{
  match_counter_->set_index(match_index);
}

void StatusLine::set_match_count(long match_count)
{
  match_counter_->set_count(match_count);
}

void StatusLine::pulse()
{
  progressbar_->pulse();
}

void StatusLine::stop_pulse()
{
  progressbar_->set_fraction(0.0);
}

} // namespace Regexxer

