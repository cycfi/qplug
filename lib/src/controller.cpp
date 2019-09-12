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

   void controller::edit_parameter(int id, double value)
   {
      _base.edit_parameter(id, value);
   }

   void controller::parameter_change(int id, double value)
   {
      _on_parameter_change[id](value);
   }
}}
