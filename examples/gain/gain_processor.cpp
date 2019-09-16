/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "gain_processor.hpp"

using namespace q::literals;

gain_processor::gain_processor(base_processor& base)
 : qplug::processor(base)
 , _gain_lp(4_Hz, sps())
{
   parameters(_gain);
}

void gain_processor::process(in_channels const& in, out_channels const& out)
{
   auto l_in = in[0];
   auto r_in = in[1];
   auto l_out = out[0];
   auto r_out = out[1];

   for (auto i : out.frames())
   {
      _gain_lp(_gain / 100);
      l_out[i] = l_in[i] * _gain_lp();
      r_out[i] = r_in[i] * _gain_lp();
   }
}





