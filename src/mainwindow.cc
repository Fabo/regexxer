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

#include "mainwindow.h"
#include "filelist.h"
#include "pcreshell.h"
#include "stringutils.h"

#include <iostream>
#include <memory>
#include <gtkmm.h>

#include <config.h>


namespace
{

const char regexxer_icon_path[] = "pixmaps" G_DIR_SEPARATOR_S "regexxer.png";

typedef Glib::RefPtr<Regexxer::FileBuffer> FileBufferPtr;


class ImageButton : public Gtk::Button
{
public:
  explicit ImageButton(const Gtk::StockID& stock_id);
  virtual ~ImageButton();
};

class CustomButton : public Gtk::Button
{
public:
  CustomButton(const Gtk::StockID& stock_id, const Glib::ustring& label, bool mnemonic = false);
  virtual ~CustomButton();
};


ImageButton::ImageButton(const Gtk::StockID& stock_id)
{
  Gtk::Image *const image = new Gtk::Image(stock_id, Gtk::ICON_SIZE_BUTTON);
  add(*Gtk::manage(image));
  image->show();
}

ImageButton::~ImageButton()
{}


CustomButton::CustomButton(const Gtk::StockID& stock_id, const Glib::ustring& label, bool mnemonic)
{
  using namespace Gtk;

  Alignment *const alignment = new Alignment(0.5, 0.5, 0.0, 0.0);
  add(*manage(alignment));

  HBox *const hbox = new HBox(false, 3);
  alignment->add(*manage(hbox));

  hbox->pack_start(*manage(new Image(stock_id, ICON_SIZE_BUTTON)), PACK_SHRINK);
  hbox->pack_start(*manage(new Label(label, mnemonic)),            PACK_SHRINK);

  show_all_children();
}

CustomButton::~CustomButton()
{}


void dummy_handler()
{
  std::cout << "foo!\n";
}

} // anonymous namespace


namespace Regexxer
{

MainWindow::MainWindow()
:
  entry_folder_         (0),
  entry_pattern_        (0),
  button_recursive_     (0),
  button_hidden_        (0),
  entry_regex_          (0),
  entry_substitution_   (0),
  button_multiple_      (0),
  button_caseless_      (0),
  filelist_             (0),
  textview_             (0),
  entry_preview_        (0),
  button_prev_file_     (0),
  button_prev_          (0),
  button_next_          (0),
  button_next_file_     (0),
  button_replace_       (0),
  button_replace_file_  (0),
  button_replace_all_   (0)
{
  using namespace Gtk;

  try
  {
    const std::string filename = Glib::build_filename(REGEXXER_DATADIR, regexxer_icon_path);
    set_icon(Gdk::Pixbuf::create_from_file(filename));
  }
  catch(const Glib::Error& e)
  {
    std::cerr << e.what() << std::endl;
  }

  set_title_filename();
  set_default_size(600, 400);

  Box *const vbox_main = new VBox();
  add(*manage(vbox_main));

  vbox_main->pack_start(*manage(create_toolbar()), PACK_SHRINK);

  Box *const vbox_interior = new VBox();
  vbox_main->pack_start(*manage(vbox_interior), PACK_EXPAND_WIDGET);
  vbox_interior->set_border_width(2);

  Box *const hbox_bottom = new HBox(false, 1);
  vbox_main->pack_start(*manage(hbox_bottom), PACK_SHRINK);
  hbox_bottom->pack_start(*manage(new ProgressBar()), PACK_SHRINK);
  hbox_bottom->pack_start(*manage(new Statusbar()), PACK_EXPAND_WIDGET);

  Paned *const paned = new HPaned();
  vbox_interior->pack_start(*manage(paned), PACK_EXPAND_WIDGET);
  vbox_interior->pack_start(*manage(create_buttonbox()), PACK_SHRINK);

  paned->pack1(*manage(create_left_pane()),  EXPAND);
  paned->pack2(*manage(create_right_pane()), EXPAND);

  show_all_children();

  entry_folder_->set_text(Util::shorten_pathname(Glib::filename_to_utf8(Glib::get_current_dir())));
  entry_pattern_->set_text("*");
  button_recursive_->set_active(true);

  signal_hide().connect_notify(SigC::slot(*filelist_, &FileList::stop_find_files));

  filelist_->signal_switch_buffer.connect(SigC::slot(*this, &MainWindow::on_filelist_switch_buffer));
}

MainWindow::~MainWindow()
{}

Gtk::Widget* MainWindow::create_toolbar()
{
  using namespace Gtk;
  using namespace Gtk::Toolbar_Helpers;

  std::auto_ptr<Toolbar> toolbar (new Toolbar());
  ToolList& tools = toolbar->tools();

  tools.push_back(StockElem(Stock::SAVE, &dummy_handler));
  tools.push_back(Space());
  tools.push_back(StockElem(Stock::UNDO, &dummy_handler));
  tools.push_back(Space());
  tools.push_back(StockElem(Stock::PREFERENCES, &dummy_handler));
  tools.push_back(Space());
  tools.push_back(StockElem(Stock::QUIT, SigC::slot(*this, &Widget::hide)));

  toolbar->set_toolbar_style(TOOLBAR_BOTH_HORIZ);

  return toolbar.release();
}

Gtk::Widget* MainWindow::create_buttonbox()
{
  using namespace Gtk;
  using SigC::bind;
  using SigC::slot;

  std::auto_ptr<Box> buttonbox (new HBox(false, 10));
  buttonbox->set_border_width(2);

  Box *const box_action = new HBox(true, 5);
  buttonbox->pack_end(*manage(box_action), PACK_SHRINK);

  Box *const box_move = new HBox(true, 5);
  buttonbox->pack_end(*manage(box_move), PACK_SHRINK);

  button_prev_file_ = new ImageButton(Stock::GOTO_FIRST);
  box_move->pack_start(*manage(button_prev_file_));

  button_prev_ = new ImageButton(Stock::GO_BACK);
  box_move->pack_start(*manage(button_prev_));

  button_next_ = new ImageButton(Stock::GO_FORWARD);
  box_move->pack_start(*manage(button_next_));

  button_next_file_ = new ImageButton(Stock::GOTO_LAST);
  box_move->pack_start(*manage(button_next_file_));

  button_replace_ = new CustomButton(Stock::CONVERT, "_Replace", true);
  box_action->pack_start(*manage(button_replace_));

  button_replace_file_ = new CustomButton(Stock::CONVERT, "_This file", true);
  box_action->pack_start(*manage(button_replace_file_));

  button_replace_all_ = new CustomButton(Stock::CONVERT, "_All files", true);
  box_action->pack_start(*manage(button_replace_all_));

  button_prev_file_   ->set_sensitive(false);
  button_prev_        ->set_sensitive(false);
  button_next_        ->set_sensitive(false);
  button_next_file_   ->set_sensitive(false);
  button_replace_     ->set_sensitive(false);
  button_replace_file_->set_sensitive(false);
  button_replace_all_ ->set_sensitive(false);

  button_prev_file_   ->signal_clicked().connect(bind(slot(*this, &MainWindow::on_go_next_file), false));
  button_prev_        ->signal_clicked().connect(bind(slot(*this, &MainWindow::on_go_next),      false));
  button_next_        ->signal_clicked().connect(bind(slot(*this, &MainWindow::on_go_next),      true));
  button_next_file_   ->signal_clicked().connect(bind(slot(*this, &MainWindow::on_go_next_file), true));
  button_replace_     ->signal_clicked().connect(slot(*this, &MainWindow::on_replace));
  button_replace_file_->signal_clicked().connect(slot(*this, &MainWindow::on_replace_file));
  button_replace_all_ ->signal_clicked().connect(slot(*this, &MainWindow::on_replace_all));

  return buttonbox.release();
}

Gtk::Widget* MainWindow::create_left_pane()
{
  using namespace Gtk;

  std::auto_ptr<VBox> vbox (new VBox(false, 3));
  vbox->set_border_width(1);

  Table *const table = new Table(3, 2, false);
  vbox->pack_start(*manage(table), PACK_SHRINK);
  table->set_border_width(1);
  table->set_spacings(2);

  Button *const button_folder = new CustomButton(Stock::OPEN, "_Folder:", true);
  table->attach(*manage(button_folder), 0, 1, 0, 1, FILL, AttachOptions(0));
  button_folder->signal_clicked().connect(SigC::slot(*this, &MainWindow::on_select_folder));

  table->attach(*manage(new Label("Pattern:", 0.0, 0.5)), 0, 1, 1, 2, FILL, AttachOptions(0));
  table->attach(*manage(entry_folder_  = new Entry()), 1, 2, 0, 1, EXPAND|FILL, AttachOptions(0));
  table->attach(*manage(entry_pattern_ = new Entry()), 1, 2, 1, 2, EXPAND|FILL, AttachOptions(0));

  HBox *const hbox = new HBox(false, 5);
  table->attach(*manage(hbox), 0, 2, 2, 3, EXPAND|FILL, AttachOptions(0));

  button_recursive_ = new CheckButton("recursive");
  hbox->pack_start(*manage(button_recursive_), PACK_SHRINK);

  button_hidden_ = new CheckButton("hidden");
  hbox->pack_start(*manage(button_hidden_), PACK_SHRINK);

  Button *const button_find = new Button(Stock::FIND);
  hbox->pack_end(*manage(button_find), PACK_SHRINK);
  button_find->signal_clicked().connect(SigC::slot(*this, &MainWindow::on_find_files));

  Frame *const frame = new Frame();
  vbox->pack_start(*manage(frame), PACK_EXPAND_WIDGET);

  ScrolledWindow *const scrollwin = new ScrolledWindow();
  frame->add(*manage(scrollwin));

  filelist_ = new FileList();
  scrollwin->add(*manage(filelist_));
  scrollwin->set_policy(POLICY_AUTOMATIC, POLICY_ALWAYS);

  tooltips_.set_tip(*entry_folder_,     "The directory to be searched.");
  tooltips_.set_tip(*entry_pattern_,    "A filename pattern as used by the shell. "
                                        "Character classes [ab] and csh style "
                                        "brace expressions {a,b} are supported.");
  tooltips_.set_tip(*button_recursive_, "recurse into subdirectories");
  tooltips_.set_tip(*button_hidden_,    "also find hidden files");

  return vbox.release();
}

Gtk::Widget* MainWindow::create_right_pane()
{
  using namespace Gtk;

  std::auto_ptr<VBox> vbox (new VBox(false, 3));
  vbox->set_border_width(1);

  Table *const table = new Table(2, 3, false);
  vbox->pack_start(*manage(table), PACK_SHRINK);
  table->set_border_width(1);
  table->set_spacings(2);

  table->attach(*manage(new Label("Search:",  0.0, 0.5)), 0, 1, 0, 1, FILL, AttachOptions(0));
  table->attach(*manage(new Label("Replace:", 0.0, 0.5)), 0, 1, 1, 2, FILL, AttachOptions(0));
  table->attach(*manage(entry_regex_        = new Entry()),  1, 2, 0, 1, EXPAND|FILL, AttachOptions(0));
  table->attach(*manage(entry_substitution_ = new Entry()),  1, 2, 1, 2, EXPAND|FILL, AttachOptions(0));

  entry_substitution_->signal_changed().connect(SigC::slot(*this, &MainWindow::update_preview));

  HBox *const hbox_options = new HBox(false, 5);
  table->attach(*manage(hbox_options), 2, 3, 0, 1, FILL, AttachOptions(0));
  hbox_options->pack_start(*manage(button_multiple_ = new CheckButton("/g")), PACK_SHRINK);
  hbox_options->pack_start(*manage(button_caseless_ = new CheckButton("/i")), PACK_SHRINK);

  Button *const button_find = new Button(Stock::FIND);
  table->attach(*manage(button_find), 2, 3, 1, 2, FILL, AttachOptions(0));
  button_find->signal_clicked().connect(SigC::slot(*this, &MainWindow::on_exec_search));

  Frame *const frame = new Frame();
  vbox->pack_start(*manage(frame), PACK_EXPAND_WIDGET);

  VBox *const vbox_textview = new VBox(false, 3);
  frame->add(*manage(vbox_textview));

  ScrolledWindow *const scrollwin = new ScrolledWindow();
  vbox_textview->pack_start(*manage(scrollwin), PACK_EXPAND_WIDGET);

  textview_ = new TextView();
  scrollwin->add(*manage(textview_));

  entry_preview_ = new Entry();
  vbox_textview->pack_start(*manage(entry_preview_), PACK_SHRINK);
  entry_preview_->set_has_frame(false);
  entry_preview_->set_editable(false);
  entry_preview_->unset_flags(CAN_FOCUS);

  const Pango::FontDescription fixed_font ("fixed");
  textview_     ->modify_font(fixed_font);
  entry_preview_->modify_font(fixed_font);

  tooltips_.set_tip(*entry_regex_,        "A regular expression in Perl syntax.");
  tooltips_.set_tip(*entry_substitution_, "The new string to substitute. As in Perl, you can "
                                          "refer to parts of the match using $1, $2, etc. "
                                          "or even $+, $&, $` and $'. The operators "
                                          "\\l, \\u, \\L, \\U and \\E are supported as well.");
  tooltips_.set_tip(*button_multiple_,    "find all possible matches in a line");
  tooltips_.set_tip(*button_caseless_,    "do case insensitive matching");

  return vbox.release();
}

void MainWindow::on_select_folder()
{
  using namespace Glib;

  Gtk::FileSelection filesel ("Select a folder");
  filesel.set_transient_for(*this);
  filesel.hide_fileop_buttons();

  std::string filename = filename_from_utf8(Util::expand_pathname(entry_folder_->get_text()));

  if(!filename.empty())
  {
    if(*filename.rbegin() != G_DIR_SEPARATOR)
      filename += G_DIR_SEPARATOR;

    filesel.set_filename(filename);
  }

  if(filesel.run() == Gtk::RESPONSE_OK)
  {
    filename = filesel.get_filename();
    entry_folder_->set_text(Util::shorten_pathname(filename_to_utf8(path_get_dirname(filename))));

    if(!filename.empty() && *filename.rbegin() != G_DIR_SEPARATOR)
      entry_pattern_->set_text(filename_to_utf8(path_get_basename(filename)));
  }
}

void MainWindow::on_find_files()
{
  filelist_->find_files(
      Util::expand_pathname(entry_folder_->get_text()),
      entry_pattern_->get_text(),
      button_recursive_->get_active(),
      button_hidden_->get_active());
}

void MainWindow::on_exec_search()
{
  button_prev_file_->set_sensitive(false);
  button_prev_     ->set_sensitive(false);
  button_next_     ->set_sensitive(false);
  button_next_file_->set_sensitive(false);

  try
  {
    pcre_pattern_.reset(new Pcre::Pattern(
        entry_regex_->get_text(),
        button_caseless_->get_active() ? Pcre::CASELESS : Pcre::CompileOptions(0)));
  }
  catch(const Pcre::Error& e)
  {
    std::cerr << e.what() << std::endl;
    return;
  }

  const long match_count = filelist_->find_matches(*pcre_pattern_, button_multiple_->get_active());

  if(match_count > 0)
  {
    filelist_->select_first_file();
    on_go_next(true);
  }
}

void MainWindow::on_filelist_switch_buffer(FileInfoPtr fileinfo, BoundState bound)
{
  const FileBufferPtr old_buffer = FileBufferPtr::cast_dynamic(textview_->get_buffer());

  if(fileinfo && fileinfo->buffer == old_buffer)
    return;

  if(old_buffer)
  {
    conn_bound_state_changed_.disconnect();
    conn_preview_changed_.disconnect();

    old_buffer->forget_current_match();
  }

  button_prev_file_->set_sensitive((bound & BOUND_FIRST) == 0);
  button_next_file_->set_sensitive((bound & BOUND_LAST)  == 0);

  if(fileinfo && fileinfo->buffer)
  {
    textview_->set_buffer(fileinfo->buffer);
    set_title_filename(Glib::filename_to_utf8(fileinfo->fullname));

    conn_bound_state_changed_ = fileinfo->buffer->signal_bound_state_changed.
        connect(SigC::slot(*this, &MainWindow::on_bound_state_changed));

    conn_preview_changed_ = fileinfo->buffer->signal_preview_line_changed.
        connect(SigC::slot(*this, &MainWindow::update_preview));

    on_bound_state_changed(fileinfo->buffer->get_bound_state());
  }
  else
  {
    textview_->set_buffer(FileBuffer::create());
    set_title_filename();

    on_bound_state_changed(BOUND_FIRST | BOUND_LAST);
  }

  update_preview();
}

void MainWindow::on_bound_state_changed(BoundState bound)
{
  button_prev_->set_sensitive((bound & BOUND_FIRST) == 0 || button_prev_file_->sensitive());
  button_next_->set_sensitive((bound & BOUND_LAST)  == 0 || button_next_file_->sensitive());
  button_replace_file_->set_sensitive((bound & BOUND_MASK) != BOUND_MASK);
}

void MainWindow::on_go_next_file(bool move_forward)
{
  filelist_->select_next_file(move_forward);
  on_go_next(move_forward);
}

void MainWindow::on_go_next(bool move_forward)
{
  if(const FileBufferPtr buffer = FileBufferPtr::cast_dynamic(textview_->get_buffer()))
  {
    if(const Glib::RefPtr<Gtk::TextMark> mark = buffer->get_next_match(move_forward))
    {
      textview_->scroll_to_mark(mark, 0.2);
      return;
    }
  }

  if(filelist_->select_next_file(move_forward))
  {
    on_go_next(move_forward); // recursive call
  }
}

void MainWindow::on_replace()
{
  if(const FileBufferPtr buffer = FileBufferPtr::cast_dynamic(textview_->get_buffer()))
  {
    buffer->replace_current_match(entry_substitution_->get_text());
  }
}

void MainWindow::on_replace_file()
{
}

void MainWindow::on_replace_all()
{
  const Glib::RefPtr<Glib::MainContext> main_context = Glib::MainContext::get_default();
  const Glib::ustring substitution = entry_substitution_->get_text();

  Glib::RefPtr<Gtk::TextMark> mark;

  do
  {
    while(main_context->iteration(false)) {}

    if(const FileBufferPtr buffer = FileBufferPtr::cast_dynamic(textview_->get_buffer()))
    {
      do
      {
        buffer->replace_current_match(substitution);
        mark = buffer->get_next_match(true);
      }
      while(mark);
    }
  }
  while(filelist_->select_next_file(true));

  if(mark)
  {
    if(const FileBufferPtr buffer = FileBufferPtr::cast_dynamic(mark->get_buffer()))
      textview_->scroll_to_mark(mark, 0.2);
  }
}

void MainWindow::update_preview()
{
  if(const FileBufferPtr buffer = FileBufferPtr::cast_dynamic(textview_->get_buffer()))
  {
    Glib::ustring preview;
    const int pos = buffer->get_line_preview(entry_substitution_->get_text(), preview);
    entry_preview_->set_text(preview);

    button_replace_    ->set_sensitive(pos >= 0);
    button_replace_all_->set_sensitive(pos >= 0);

    // Beware, strange code ahead!
    //
    // The goal is to scroll the preview entry so that it shows the entire
    // replaced text if possible.  In order to do that we first move the cursor
    // to 0, forcing scrolling to the left boundary.  Then we set the cursor to
    // the end of the replaced text, thus forcing the entry widget to scroll
    // again.  The replacement should then be entirely visible provided that it
    // fits into the entry.
    //
    // The problem is that Gtk::Entry doesn't update its scroll position
    // immediately but in an idle handler, thus any calls to set_position()
    // but the last one have no effect at all.
    //
    // To workaround that, we install an idle handler that's executed just
    // after the entry updated its scroll position, but before redrawing is
    // done.

    entry_preview_->set_position(0);

    if(pos > 0)
    {
      using SigC::slot;
      using SigC::bind;
      using SigC::bind_return;

      Glib::signal_idle().connect(
          bind_return(bind(slot(*entry_preview_, &Gtk::Editable::set_position), pos), false),
          Glib::PRIORITY_HIGH_IDLE + 17); // between scroll update (+ 15) and redraw (+ 20)
    }
  }
}

void MainWindow::set_title_filename(const Glib::ustring& filename)
{
  Glib::ustring title;

  if(!filename.empty())
  {
    title  = Glib::path_get_basename(filename);
    title += " (";
    title += Util::shorten_pathname(Glib::path_get_dirname(filename));
    title += ") - ";
  }

  title += PACKAGE_NAME;

  set_title(title);
}

} // namespace Regexxer

