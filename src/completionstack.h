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

#ifndef REGEXXER_COMPLETIONSTACK_H_INCLUDED
#define REGEXXER_COMPLETIONSTACK_H_INCLUDED

#include <gtkmm/treemodel.h>

#include <list>

namespace Gtk
{
class ListStore;
}


namespace Regexxer
{

class CompletionStack
{
public:
  CompletionStack(const std::list<Glib::ustring>& stack = std::list<Glib::ustring>());
  CompletionStack(unsigned int stack_size, const std::list<Glib::ustring>& stack = std::list<Glib::ustring>());
  virtual ~CompletionStack();
  
  void push(const Glib::ustring value);
  void push(const std::list<Glib::ustring> values);
  
  Glib::RefPtr<Gtk::ListStore> get_completion_model();
  Gtk::TreeModelColumn<Glib::ustring> get_completion_column();
  
  std::list<Glib::ustring> get_stack();
  
protected:
  CompletionStack(const CompletionStack&);
  CompletionStack operator=(const CompletionStack&);

private:
  class CompletionColumnModel : public Gtk::TreeModel::ColumnRecord
  {
  public:
    CompletionColumnModel()
    {
      add(value_);
    }

    Gtk::TreeModelColumn<Glib::ustring> value_;
  };
  
  bool infinite_stack_;
  unsigned int stack_size_;
  CompletionColumnModel completion_column_model_;
  Glib::RefPtr<Gtk::ListStore> completion_model_;
};

} // namespace Regexxer

#endif /* REGEXXER_COMPLETIONSTACK_H_INCLUDED */
