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

#include "filetreeprivate.h"

#include <sigc++/class_slot.h>
#include <gtkmm/treestore.h>

#include <glib.h>
#include <cstring> /* for fucked up libstdc++-v2, see collatekey_sort_func() */


namespace Regexxer
{

namespace FileTreePrivate
{

const FileTreeColumns& filetree_columns()
{
  static FileTreeColumns column_record;
  return column_record;
}

int default_sort_func(const Gtk::TreeModel::iterator& lhs, const Gtk::TreeModel::iterator& rhs)
{
  const FileTreeColumns& columns = filetree_columns();

  const std::string lhs_key ((*lhs)[columns.collatekey]);
  const std::string rhs_key ((*rhs)[columns.collatekey]);

  return lhs_key.compare(rhs_key);
}

int collatekey_sort_func(const Gtk::TreeModel::iterator& lhs, const Gtk::TreeModel::iterator& rhs)
{
  const FileTreeColumns& columns = filetree_columns();

  const std::string lhs_key ((*lhs)[columns.collatekey]);
  const std::string rhs_key ((*rhs)[columns.collatekey]);

  if(lhs_key.empty() || rhs_key.empty())
    return (lhs_key.empty() && rhs_key.empty()) ? 0 : (lhs_key.empty() ? -1 : 1);

  // Can't use the following absolutely correct code due to the fucked up
  // libstdc++-v2 GCC 2.95.x comes with.  Resort to strcmp().  Damn.
  //
  // return lhs_key.compare(1, std::string::npos, rhs_key, 1, std::string::npos);

  return std::strcmp(lhs_key.c_str() + 1, rhs_key.c_str() + 1);
}

} // namespace FileTreePrivate


/**** Regexxer::FileTree::TreeRowRef ***************************************/

FileTree::TreeRowRef::TreeRowRef(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreePath& path)
:
  Gtk::TreeRowReference(model, path)
{}

FileTree::TreeRowRef::~TreeRowRef()
{}


/**** Regexxer::FileTree::MessageList **************************************/

FileTree::MessageList::MessageList()
{}

FileTree::MessageList::~MessageList()
{}


/**** Regexxer::FileTree::Error ********************************************/

FileTree::Error::Error(const Util::SharedPtr<FileTree::MessageList>& error_list)
:
  error_list_ (error_list)
{}

FileTree::Error::~Error()
{}

FileTree::Error::Error(const FileTree::Error& other)
:
  error_list_ (other.error_list_)
{}

FileTree::Error& FileTree::Error::operator=(const FileTree::Error& other)
{
  error_list_ = other.error_list_;
  return *this;
}

const std::list<Glib::ustring>& FileTree::Error::get_error_list() const
{
  return *error_list_;
}


/**** Regexxer::FileTree::FindData *****************************************/

FileTree::FindData::FindData(Pcre::Pattern& pattern_, bool recursive_, bool hidden_)
:
  pattern     (pattern_),
  recursive   (recursive_),
  hidden      (hidden_),
  error_list  (new FileTree::MessageList())
{}

FileTree::FindData::~FindData()
{}


/**** Regexxer::FileTree::FindMatchesData **********************************/

FileTree::FindMatchesData::FindMatchesData(Pcre::Pattern& pattern_, bool multiple_)
:
  pattern              (pattern_),
  multiple             (multiple_),
  path_match_first_set (false)
{}


/**** Regexxer::FileTree::ReplaceMatchesData *******************************/

FileTree::ReplaceMatchesData::ReplaceMatchesData(FileTree& filetree_,
                                                 const Glib::ustring& substitution_)
:
  filetree             (filetree_),
  substitution         (substitution_),
  undo_stack           (new UndoStack()),
  slot_undo_stack_push (SigC::slot_class(*this, &FileTree::ReplaceMatchesData::undo_stack_push))
{}

FileTree::ReplaceMatchesData::~ReplaceMatchesData()
{}

void FileTree::ReplaceMatchesData::undo_stack_push(UndoActionPtr undo_action)
{
  g_return_if_fail(row_reference);

  undo_stack->push(UndoActionPtr(new BufferActionShell(filetree, row_reference, undo_action)));
}


/**** Regexxer::FileTree::ScopedBlockSorting *******************************/

FileTree::ScopedBlockSorting::ScopedBlockSorting(FileTree& filetree)
:
  filetree_     (filetree),
  sort_column_  (Gtk::TreeStore::DEFAULT_SORT_COLUMN_ID),
  sort_order_   (Gtk::SORT_ASCENDING)
{
  filetree_.set_headers_clickable(false);
  filetree_.treestore_->get_sort_column_id(sort_column_, sort_order_);

  // If we're currently sorting on the match count column, we have to switch
  // temporarily to the default sort column because changes to the match count
  // could cause reordering of the model.  Gtk::TreeModel::foreach() won't
  // like that at all, and that's precisily why this utility class exists.
  //
  if(sort_column_ == FileTreePrivate::filetree_columns().matchcount.index())
    filetree_.treestore_->set_sort_column_id(Gtk::TreeStore::DEFAULT_SORT_COLUMN_ID, sort_order_);
}

FileTree::ScopedBlockSorting::~ScopedBlockSorting()
{
  filetree_.treestore_->set_sort_column_id(sort_column_, sort_order_);
  filetree_.set_headers_clickable(true);
}


/**** Regexxer::FileTree::BufferActionShell ********************************/

FileTree::BufferActionShell::BufferActionShell(
    FileTree& filetree, const FileTree::TreeRowRefPtr& row_reference,
    const UndoActionPtr& buffer_action)
:
  filetree_      (filetree),
  row_reference_ (row_reference),
  buffer_action_ (buffer_action)
{}

FileTree::BufferActionShell::~BufferActionShell()
{}

bool FileTree::BufferActionShell::do_undo()
{
  g_return_val_if_fail(row_reference_->is_valid(), false);

  const Gtk::TreePath path (row_reference_->get_path());

  if(!filetree_.last_selected_rowref_ ||
      filetree_.last_selected_rowref_->get_path() != path)
  {
    filetree_.expand_and_select(path);
  }

  return buffer_action_->undo();
}

} // namespace Regexxer

