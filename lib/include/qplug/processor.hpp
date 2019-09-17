/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#pragma once

#include <qplug/parameter.hpp>
#include <q/support/audio_stream.hpp>
#include <memory>
#include <vector>
#include <type_traits>

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

                              template <typename... T>
      void                    parameters(T&... param);

   private:

      friend base_processor;

      void                    parameter_change(int id, double value);

                              template <typename T, typename... Rest>
      void                    add_parameter(int id, T& param, Rest&... rest);

      using param_change = std::function<void(double)>;
      using parameter_change_list = std::vector<param_change>;

      base_processor&         _base;
      parameter_change_list   _on_parameter_change;
   };

   using processor_ptr = std::unique_ptr<processor>;
   processor_ptr make_processor(base_processor& base);

   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename... Rest>
   inline void processor::add_parameter(int id, T& param, Rest&... rest)
   {
      _on_parameter_change.push_back(
         [this, &param](double value)
         {
            if constexpr(std::is_same<T, bool>::value)
               param = value > 0.5;
            else
               param = value;
         }
      );

      if constexpr(sizeof...(rest) != 0)
         add_parameter(id+1, rest...);
   }

   template <typename... T>
   inline void processor::parameters(T&... param)
   {
      _on_parameter_change.clear();
      add_parameter(0, param...);
   }
}}
