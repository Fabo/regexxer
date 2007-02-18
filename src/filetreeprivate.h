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

#ifndef REGEXXER_FILETREEPRIVATE_H_INCLUDED
#define REGEXXER_FILETREEPRIVATE_H_INCLUDED

#include "filetree.h"

#include <gtkmm/treerowreference.h>
#include <gtkmm/treestore.h>
#include <utility>

namespace Regexxer
{

/*
 * This namespace contains all types and functions which are used exclusively by
 * the FileTree implementation, but aren't members of class Regexxer::FileTree.
 */
namespace FileTreePrivate
{

// Normally I don't like global using declarations, but it's
// reasonable to make an exception for the smart pointer casts.
using Util::shared_dynamic_cast;
using Util::shared_polymorphic_cast;

struct FileTreeColumns : public Gtk::TreeModel::ColumnRecord
{
  Gtk::TreeModelColumn<Glib::ustring>   filename;
  Gtk::TreeModelColumn<std::string>     collatekey;
  Gtk::TreeModelColumn<int>             matchcount;
  Gtk::TreeModelColumn<FileInfoBasePtr> fileinfo;

  static const FileTreeColumns& instance();

private:
  FileTreeColumns() { add(filename); add(collatekey); add(matchcount); add(fileinfo); }
};

inline
FileInfoPtr get_fileinfo_from_iter(const Gtk::TreeModel::iterator& iter)
{
  const FileInfoBasePtr base ((*iter)[FileTreeColumns::instance().fileinfo]);
  return shared_dynamic_cast<FileInfo>(base);
}

int default_sort_func   (const Gtk::TreeModel::iterator& a, const Gtk::TreeModel::iterator& b);
int collatekey_sort_func(const Gtk::TreeModel::iterator& a, const Gtk::TreeModel::iterator& b);

bool next_match_file(Gtk::TreeModel::iterator& iter, Gtk::TreeModel::Path* collapse = 0);
bool prev_match_file(Gtk::TreeModel::iterator& iter, Gtk::TreeModel::Path* collapse = 0);

typedef std::pair<std::string, Gtk::TreeModel::iterator> DirNodePair;
typedef std::list<DirNodePair>                           DirStack;

class ScopedPushDir
{
private:
  DirStack& dirstack_;

  ScopedPushDir(const ScopedPushDir&);
  ScopedPushDir& operator=(const ScopedPushDir&);

public:
  ScopedPushDir(DirStack& dirstack, const std::string& dirname)
    : dirstack_ (dirstack)
    { dirstack_.push_back(DirNodePair(dirname, Gtk::TreeModel::iterator())); }

  ~ScopedPushDir() { dirstack_.pop_back(); }
};

} // namespace FileTreePrivate

/* This is just a Gtk::TreeRowReference wrapper that can be used with Util::SharedPtr<>.
 */
class FileTree::TreeRowRef : public Util::SharedObject, public Gtk::TreeRowReference
{
public:
  TreeRowRef(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::Path& path);
  ~TreeRowRef();

private:
  TreeRowRef(const FileTree::TreeRowRef&);
  FileTree::TreeRowRef& operator=(const FileTree::TreeRowRef&);
};

/* This is just a std::list<> wrapper that can be used with Util::SharedPtr<>.
 */
class FileTree::MessageList : public Util::SharedObject, public std::list<Glib::ustring>
{
public:
  MessageList();
  ~MessageList();

private:
  MessageList(const FileTree::MessageList&);
  FileTree::MessageList& operator=(const FileTree::MessageList&);
};

struct FileTree::FindData
{
  FindData(Pcre::Pattern& pattern_, bool recursive_, bool hidden_);
  ~FindData();

  Pcre::Pattern&                          pattern;
  const bool                              recursive;
  const bool                              hidden;
  FileTreePrivate::DirStack               dirstack;
  Util::SharedPtr<FileTree::MessageList>  error_list;

private:
  FindData(const FileTree::FindData&);
  FileTree::FindData& operator=(const FileTree::FindData&);
};

struct FileTree::FindMatchesData
{
  FindMatchesData(Pcre::Pattern& pattern_, bool multiple_);

  Pcre::Pattern&  pattern;
  const bool      multiple;
  bool            path_match_first_set;

private:
  FindMatchesData(const FileTree::FindMatchesData&);
  FileTree::FindMatchesData& operator=(const FileTree::FindMatchesData&);
};

struct FileTree::ReplaceMatchesData
{
  ReplaceMatchesData(FileTree& filetree_, const Glib::ustring& substitution_);
  ~ReplaceMatchesData();

  FileTree&                             filetree;
  const Glib::ustring                   substitution;
  FileTree::TreeRowRefPtr               row_reference;
  UndoStackPtr                          undo_stack;
  const sigc::slot<void, UndoActionPtr> slot_undo_stack_push;

  void undo_stack_push(UndoActionPtr undo_action);

private:
  ReplaceMatchesData(const FileTree::ReplaceMatchesData&);
  FileTree::ReplaceMatchesData& operator=(const FileTree::ReplaceMatchesData&);
};

class FileTree::ScopedBlockSorting
{
public:
  explicit ScopedBlockSorting(FileTree& filetree);
  ~ScopedBlockSorting();

private:
  FileTree&     filetree_;
  int           sort_column_;
  Gtk::SortType sort_order_;

  ScopedBlockSorting(const FileTree::ScopedBlockSorting&);
  FileTree::ScopedBlockSorting& operator=(const FileTree::ScopedBlockSorting&);
};

class FileTree::BufferActionShell : public UndoAction
{
public:
  BufferActionShell(FileTree& filetree, const FileTree::TreeRowRefPtr& row_reference,
                    const UndoActionPtr& buffer_action);
  virtual ~BufferActionShell();

private:
  FileTree&               filetree_;
  FileTree::TreeRowRefPtr row_reference_;
  UndoActionPtr           buffer_action_;

  virtual bool do_undo(const sigc::slot<bool>& pulse);
};

} // namespace Regexxer

#endif /* REGEXXER_FILETREEPRIVATE_H_INCLUDED */
