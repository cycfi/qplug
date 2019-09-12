/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#pragma once

#include <qplug/parameter.hpp>
#include <q/support/audio_stream.hpp>
#include <memory>

#if defined(IPLUG2)
# include "IPlug_include_in_plug_hdr.h"
class iplug2_plugin;
using base_processor = iplug2_plugin;
#endif

namespace cycfi { namespace qplug
{
   class processor : public q::audio_stream
   {
   public:
                              processor(iplug2_plugin& base)
                               : _base(base)
                              {}
                              processor(processor const&) = delete;
      virtual                 ~processor() {}

      virtual void            reset() {}
      virtual void            activate() const {}
      virtual void            deactivate() const {}

      virtual std::uint32_t   tail_samples() const { return 0; }
      virtual std::uint32_t   latency_samples() const { return 0; }

      std::uint32_t           sps() const;
      bool                    bypassed() const;

   private:

      base_processor&         _base;
   };

   using processor_ptr = std::unique_ptr<processor>;
   processor_ptr make_processor(base_processor& base);
}}
