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

   void processor::parameter_change(int id, double value)
   {
      _on_parameter_change[id](value);
   }
}}
