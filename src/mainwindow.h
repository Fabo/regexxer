
#ifndef REGEXXER_MAINWINDOW_H_INCLUDED
#define REGEXXER_MAINWINDOW_H_INCLUDED

#include <memory>
#include <gtkmm/tooltips.h>
#include <gtkmm/window.h>

#include "filebuffer.h"
#include "sharedptr.h"


namespace Gtk
{
class Button;
class CheckButton;
class Entry;
class TextBuffer;
class TextView;
}

namespace Pcre { class Pattern; }

namespace Regexxer
{

class FileBuffer;
class FileList;
struct FileInfo;

class MainWindow : public Gtk::Window
{
public:
  MainWindow();
  virtual ~MainWindow();

private:
  Gtk::Tooltips     tooltips_;

  Gtk::Entry*       entry_folder_;
  Gtk::Entry*       entry_pattern_;
  Gtk::CheckButton* button_recursive_;
  Gtk::CheckButton* button_hidden_;

  Gtk::Entry*       entry_regex_;
  Gtk::Entry*       entry_substitution_;
  Gtk::CheckButton* button_multiple_;
  Gtk::CheckButton* button_caseless_;

  FileList*         filelist_;
  Gtk::TextView*    textview_;
  Gtk::Entry*       entry_preview_;
  SigC::Connection  conn_bound_state_changed_;
  SigC::Connection  conn_preview_changed_;

  Gtk::Button*      button_prev_file_;
  Gtk::Button*      button_prev_;
  Gtk::Button*      button_next_;
  Gtk::Button*      button_next_file_;
  Gtk::Button*      button_replace_;
  Gtk::Button*      button_replace_file_;
  Gtk::Button*      button_replace_all_;

  std::auto_ptr<Pcre::Pattern> pcre_pattern_;

  Gtk::Widget* create_toolbar();
  Gtk::Widget* create_buttonbox();
  Gtk::Widget* create_left_pane();
  Gtk::Widget* create_right_pane();

  void on_select_folder();
  void on_find_files();
  void on_exec_search();

  void on_filelist_switch_buffer(Util::SharedPtr<FileInfo> fileinfo, BoundState bound);
  void on_bound_state_changed(BoundState bound);

  void on_go_next_file(bool move_forward);
  void on_go_next(bool move_forward);
  void on_replace();
  void on_replace_file();
  void on_replace_all();

  void update_preview();
  void set_title_filename(const Glib::ustring& filename = Glib::ustring());
};

} // namespace Regexxer

#endif

