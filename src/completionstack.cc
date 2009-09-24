/*
 * Copyright (c) 2009  Fabien Parent  <parent.f@gmail.com>
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

#include "completionstack.h"

#include <gtkmm/liststore.h>

namespace Regexxer
{

CompletionStack::CompletionStack(const std::list<Glib::ustring>& stack) :
  infinite_stack_(true),
  stack_size_(0),
  completion_column_model_(),
  completion_model_(Gtk::ListStore::create(completion_column_model_))
{
  push(stack);
}

CompletionStack::CompletionStack(unsigned int stack_size, const std::list<Glib::ustring>& stack) :
  infinite_stack_(false),
  stack_size_(stack_size),
  completion_column_model_(),
  completion_model_(Gtk::ListStore::create(completion_column_model_))
{
  push(stack);
}

void CompletionStack::push(const std::list<Glib::ustring> values)
{
  for (std::list<Glib::ustring>::const_reverse_iterator item = values.rbegin();
       item != values.rend(); item++)
  {
    push(*item);
  }
}

void CompletionStack::push(const Glib::ustring value)
{
  if (value.empty())
    return;
  
  Gtk::ListStore::Children children = completion_model_->children();
  for (Gtk::ListStore::Children::iterator i = children.begin(); i != children.end(); i++)
  {
    if (i->get_value(completion_column_model_.value_) == value)
    {
      completion_model_->move(i, children.begin());
      return;
    }
  }
  
  Gtk::TreeModel::Row row = *(completion_model_->prepend());
  row[completion_column_model_.value_] = value;
  
  if (!infinite_stack_ && children.size() > stack_size_)
  {
    Gtk::ListStore::Children::iterator last_element = --(children.end());
    completion_model_->erase(last_element);
  }
}

std::list<Glib::ustring> CompletionStack::get_stack()
{
  std::list<Glib::ustring> return_stack;
  Gtk::ListStore::Children children = completion_model_->children();
  for (Gtk::ListStore::Children::iterator i = children.begin(); i != children.end(); i++)
  {
    return_stack.push_back(i->get_value(completion_column_model_.value_));
  }
  
  return return_stack;
}

Glib::RefPtr<Gtk::ListStore> CompletionStack::get_completion_model()
{
  return completion_model_;
}

Gtk::TreeModelColumn<Glib::ustring> CompletionStack::get_completion_column()
{
	return completion_column_model_.value_;
}

CompletionStack::~CompletionStack()
{
  
}

} // namespace Regexxer
