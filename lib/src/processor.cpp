/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <qplug/processor.hpp>

#if defined(IPLUG2)
# include "iplug2/iplug2_plugin.hpp"
#endif

namespace cycfi { namespace qplug
{
   std::uint32_t processor::sps() const
   {
      return _base.GetSampleRate();
   }

   bool processor::bypassed() const
   {
      return _base.GetBypassed();
   }

   void processor::update_parameter(int id, double value)
   {
      if (id < _on_parameter_change.size())
         _on_parameter_change[id](value);
   }

   void processor::parameter_change(int id, double value)
   {
      update_parameter(id, value);
      on_parameter_change(id, value);
   }
}}
