/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <qplug/controller.hpp>

#if defined(IPLUG2)
# include "iplug2/iplug2_plugin.hpp"
#endif

namespace cycfi { namespace qplug
{
   elements::view* controller::view() const
   {
      return _base.view();
   }

   void controller::edit_parameter(int id, double value, bool notify_self)
   {
      _base.edit_parameter(id, value);
      if (notify_self)
         on_edit_parameter(id, value);
   }

   void controller::update_ui_parameter(int id, double value)
   {
      if (id < _on_parameter_change.size())
         _on_parameter_change[id](value);
   }

   void controller::parameter_change(int id, double value)
   {
      update_ui_parameter(id, value);
      on_parameter_change(id, value);
   }

   double controller::get_parameter(int id)
   {
      return _base.get_parameter(id);
   }

   double controller::get_parameter_normalized(int id)
   {
      return _base.get_parameter_normalized(id);
   }
}}
