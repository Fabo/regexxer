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

#include "filetreeprivate.h"

#include <glib.h>
#include <gtkmm/treestore.h>

namespace Regexxer
{

namespace FileTreePrivate
{

// static
const FileTreeColumns& FileTreeColumns::instance()
{
  static FileTreeColumns column_record;
  return column_record;
}

int default_sort_func(const Gtk::TreeModel::iterator& a, const Gtk::TreeModel::iterator& b)
{
  const FileTreeColumns& columns = FileTreeColumns::instance();

  const std::string a_key = (*a)[columns.collatekey];
  const std::string b_key = (*b)[columns.collatekey];

  return a_key.compare(b_key);
}

int collatekey_sort_func(const Gtk::TreeModel::iterator& a, const Gtk::TreeModel::iterator& b)
{
  typedef std::string::difference_type diff_type;

  const FileTreeColumns& columns = FileTreeColumns::instance();

  const std::string a_key = (*a)[columns.collatekey];
  const std::string b_key = (*b)[columns.collatekey];

  if (a_key.size() > 1 && b_key.size() > 1)
    return a_key.compare(1, std::string::npos, b_key, 1, std::string::npos);
  else
    return diff_type(a_key.size()) - diff_type(b_key.size());
}

bool next_match_file(Gtk::TreeModel::iterator& iter, Gtk::TreeModel::Path* collapse)
{
  g_return_val_if_fail(iter, false);

  const FileTreeColumns& columns = FileTreeColumns::instance();
  Gtk::TreeModel::iterator parent = iter->parent();
  ++iter;

  for (;;)
  {
    if (iter)
    {
      if ((*iter)[columns.matchcount] > 0)
      {
        if (const Gtk::TreeModel::Children& children = iter->children()) // directory?
        {
          parent = iter;
          iter = children.begin();
          continue;
        }

        return true;
      }
    }
    else if (parent)
    {
      iter = parent;
      parent = iter->parent();

      if (collapse)
        *collapse = iter;
    }
    else
      break;

    ++iter;
  }

  return false;
}

bool prev_match_file(Gtk::TreeModel::iterator& iter, Gtk::TreeModel::Path* collapse)
{
  g_return_val_if_fail(iter, false);

  const FileTreeColumns& columns = FileTreeColumns::instance();
  Gtk::TreeModel::iterator parent = iter->parent();
  Gtk::TreeModel::Path path (iter);

  for (;;)
  {
    if (path.prev())
    {
      iter = parent->children()[path.back()];

      if ((*iter)[columns.matchcount] > 0)
      {
        if (const Gtk::TreeModel::Children& children = iter->children()) // directory?
        {
          parent = iter;
          path.push_back(children.size()); // one beyond the last child
          continue;
        }

        return true;
      }
    }
    else if (parent)
    {
      parent = parent->parent();
      path.up();

      if (collapse)
        *collapse = path;
    }
    else
      break;
  }

  return false;
}

} // namespace FileTreePrivate

/**** Regexxer::FileTree::TreeRowRef ***************************************/

FileTree::TreeRowRef::TreeRowRef(const Glib::RefPtr<Gtk::TreeModel>& model,
                                 const Gtk::TreeModel::Path& path)
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
  pattern    (pattern_),
  recursive  (recursive_),
  hidden     (hidden_),
  error_list (new FileTree::MessageList())
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
  slot_undo_stack_push (sigc::mem_fun(*this, &FileTree::ReplaceMatchesData::undo_stack_push))
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
  // like that at all, and that's precisely why this utility class exists.
  //
  if (sort_column_ == FileTreePrivate::FileTreeColumns::instance().matchcount.index())
    filetree_.treestore_->set_sort_column(Gtk::TreeStore::DEFAULT_SORT_COLUMN_ID, sort_order_);
}

FileTree::ScopedBlockSorting::~ScopedBlockSorting()
{
  filetree_.treestore_->set_sort_column(sort_column_, sort_order_);
  filetree_.set_headers_clickable(true);
}

/**** Regexxer::FileTree::BufferActionShell ********************************/

FileTree::BufferActionShell::BufferActionShell(FileTree& filetree,
                                               const FileTree::TreeRowRefPtr& row_reference,
                                               const UndoActionPtr& buffer_action)
:
  filetree_      (filetree),
  row_reference_ (row_reference),
  buffer_action_ (buffer_action)
{}

FileTree::BufferActionShell::~BufferActionShell()
{}

bool FileTree::BufferActionShell::do_undo(const sigc::slot<bool>& pulse)
{
  g_return_val_if_fail(row_reference_->is_valid(), false);

  const Gtk::TreeModel::Path path = row_reference_->get_path();

  if (!filetree_.last_selected_rowref_
      || filetree_.last_selected_rowref_->get_path() != path)
  {
    filetree_.expand_and_select(path);
  }

  return buffer_action_->undo(pulse);
}

} // namespace Regexxer
