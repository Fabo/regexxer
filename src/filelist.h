
#ifndef REGEXXER_FILELIST_H_INCLUDED
#define REGEXXER_FILELIST_H_INCLUDED

#include <iosfwd>
#include <gtkmm/treepath.h>
#include <gtkmm/treeview.h>

#include "filebuffer.h"
#include "sharedptr.h"

namespace Gtk  { class ListStore; }
namespace Pcre { class Pattern;   }


namespace Regexxer
{

struct FileInfo : public Util::SharedObject
{
  std::string              fullname;
  Glib::ustring            encoding;
  Glib::RefPtr<FileBuffer> buffer;

  explicit FileInfo(const std::string& fullname_);
  ~FileInfo();
};

typedef Util::SharedPtr<FileInfo> FileInfoPtr;


class FileList : public Gtk::TreeView
{
public:
  FileList();
  virtual ~FileList();

  void find_files(const Glib::ustring& dirname,
                  const Glib::ustring& pattern,
                  bool recursive, bool hidden);
  void stop_find_files();

  void select_first_file();
  bool select_next_file(bool move_forward);

  long find_matches(Pcre::Pattern& pattern, bool multiple);

  SigC::Signal2<void,FileInfoPtr,BoundState> signal_switch_buffer;

private:
  struct FindData;

  Glib::RefPtr<Gtk::ListStore>  liststore_;
  bool                          find_running_;
  bool                          find_stop_;
  long                          sum_matches_;
  SigC::Connection              conn_match_count_;
  Gtk::TreePath                 path_match_first_;
  Gtk::TreePath                 path_match_last_;

  void find_recursively(const std::string& dirname, FindData& find_data);

  void on_selection_changed();
  void on_buffer_match_count_changed(int match_count);

  void load_file(const Util::SharedPtr<FileInfo>& fileinfo);
  Glib::RefPtr<FileBuffer> load_stream(std::istream& input);
  Glib::RefPtr<FileBuffer> convert_stream(std::istream& input, Glib::IConv& iconv);
};

} // namespace Regexxer

#endif

