
#include <glib.h>
#include "sharedptr.h"


namespace
{

int instance_count = 0;

} // anonymous namespace


namespace Util
{

SharedObject::SharedObject()
:
  refcount_ (0)
{
  ++instance_count;
  //g_print("SharedObject ctor: %d\n", instance_count);
}

SharedObject::~SharedObject()
{
  --instance_count;
  //g_print("SharedObject dtor: %d\n", instance_count);

  g_return_if_fail(refcount_ == 0);
}

} // namespace Util

