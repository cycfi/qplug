/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_GAIN_PROCESSOR_JUNE_3_2019)
#define QPLUG_GAIN_PROCESSOR_JUNE_3_2019

#include <qplug/processor.hpp>
#include <q/fx/lowpass.hpp>

namespace qplug = cycfi::qplug;
namespace q = cycfi::q;

///////////////////////////////////////////////////////////////////////////////
class gain_processor : public qplug::processor
{
public:
                        gain_processor(base_processor& base);

   void                 reset() override;
   void                 process(in_channels const& in, out_channels const& out) override;

private:

   double               _gain = 1.0;
   q::one_pole_lowpass  _gain_lp;
};

#endif

