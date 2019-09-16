/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#pragma once

#include <qplug/parameter.hpp>
#include <infra/iterator_range.hpp>
#include <elements/view.hpp>
#include <elements/element/button.hpp>
#include <memory>
#include <vector>

#if defined(IPLUG2)
# include "IPlug_include_in_plug_hdr.h"
class iplug2_plugin;
using base_controller = iplug2_plugin;
#endif

namespace cycfi { namespace qplug
{
   class controller
   {
   public:
                              controller(base_controller& base)
                               : _base(base)
                              {}
                              controller(controller const&) = delete;
      virtual                 ~controller() = default;

      virtual void            on_attach_view() {}
      virtual void            on_detach_view() {}
      elements::view*         view() const;

      using parameter_list = iterator_range<parameter*>;

      virtual parameter_list  parameters() = 0;

                              template <typename... T>
      void                    controls(T&... control);

   private:

      friend base_controller;

      void                    edit_parameter(int id, double value);
      void                    parameter_change(int id, double value);

                              template <typename T, typename... Rest>
      void                    add_controller(int id, T& first, Rest&... rest);

      using param_change = std::function<void(double)>;
      using param_change_list = std::vector<param_change>;

      base_controller&        _base;
      param_change_list       _on_parameter_change;
   };

   using controller_ptr = std::unique_ptr<controller>;
   controller_ptr make_controller(base_controller& base);

   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      template <typename FD, typename F>
      void assign_callback(FD& dest, F&& f)
      {
         if (!dest)
         {
            dest = std::forward<F>(f);
         }
         else
         {
            dest = [f1=dest, f2=f](double val)
            {
               // Chain the calls
               f1(val);
               f2(val);
            };
         }
      }

      template <typename Element, typename F>
      void set_callback(Element& e, F&& f)
      {
         assign_callback(e.on_change, std::forward<F>(f));
      }

      template <typename F>
      void set_callback(elements::basic_button& e, F&& f)
      {
         assign_callback(e.on_click, std::forward<F>(f));
      }
   }

   template <typename T, typename... Rest>
   inline void controller::add_controller(int id, T& control, Rest&... rest)
   {
      detail::set_callback(*control,
         [this, id](auto val)
         {
            edit_parameter(id, val);
         }
      );

      auto const& param = parameters()[id];
      param_change f;
      switch (param._type)
      {
         case parameter::bool_:
            f = [this, control](double value)
            {
               control->value(bool(value));
               view()->refresh(*control);
            };
            break;
         case parameter::int_:
            f = [this, control](double value)
            {
               control->value(int(value));
               view()->refresh(*control);
            };
            break;
         case parameter::double_:
            f = [this, control](double value)
            {
               control->value(value);
               view()->refresh(*control);
            };
            break;
      }
      _on_parameter_change.push_back(f);

      if constexpr(sizeof...(rest))
         add_controller(id+1, rest...);
   }

   template <typename... T>
   inline void controller::controls(T&... control)
   {
      _on_parameter_change.clear();
      add_controller(0, control...);
   }
}}
