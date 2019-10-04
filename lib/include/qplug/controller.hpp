/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_CONTROLLER_HPP_OCTOBER_17_2016)
#define QPLUG_CONTROLLER_HPP_OCTOBER_17_2016

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
      void                    controls(T&&... control);

      void                    edit_parameter(int id, double value, bool notify_self = true);
      virtual void            on_edit_parameter(int id, double value) {}
      virtual void            on_parameter_change(int id, double value) {}
      virtual void            update_ui_parameter(int id, double value);

   private:

      friend base_controller;

      void                    parameter_change(int id, double value);

                              template <typename T, typename... Rest>
      void                    add_controller(int id, T&& first, Rest&&... rest);

      using param_change = std::function<void(double)>;
      using param_change_list = std::vector<param_change>;

      base_controller&        _base;
      param_change_list       _on_parameter_change;
   };

   using controller_ptr = std::unique_ptr<controller>;
   controller_ptr make_controller(base_controller& base);

   template <typename Ptr>
   struct virtual_controller
    : std::enable_shared_from_this<virtual_controller<Ptr>>
   {
                     virtual_controller(Ptr ptr)
                      : _ptr(ptr)
                     {}

      virtual void   set_on_change(std::function<void(double)> const& f) = 0;
      virtual void   value(double val) = 0;

      Ptr            _ptr;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      template <typename FD, typename F>
      inline void assign_callback(FD& dest, F&& f)
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

      template <typename T>
      struct is_button
      {
         static constexpr bool value =
            std::is_base_of<elements::basic_button, T>::value ||
            std::is_base_of<elements::layered_button, T>::value
            ;
      };

      template <typename Element, typename F>
      inline typename std::enable_if<std::is_base_of<elements::element, Element>::value>::type
      set_callback(Element& e, F&& f)
      {
         if constexpr(is_button<Element>::value)
            assign_callback(e.on_click, std::forward<F>(f));
         else
            assign_callback(e.on_change, std::forward<F>(f));
      }

      template <typename Ptr, typename F>
      inline void set_callback(virtual_controller<Ptr>& e, F&& f)
      {
         e.set_on_change(std::forward<F>(f));
      }

      template <typename Element>
      inline typename std::enable_if<std::is_base_of<elements::element, Element>::value>::type
      refresh_element(elements::view& view_, Element& e)
      {
         view_.refresh(e);
      }

      template <typename Ptr>
      inline void refresh_element(elements::view& view_, virtual_controller<Ptr>& e)
      {
         refresh_element(view_, *e._ptr);
      }
   }

   template <typename T, typename... Rest>
   inline void controller::add_controller(int id, T&& control, Rest&&... rest)
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
               control->value(value > 0.5);
               detail::refresh_element(*view(), *control);
            };
            break;

         case parameter::int_:
         case parameter::double_:
         case parameter::frequency:
            f = [this, control](double value)
            {
               control->value(value);
               detail::refresh_element(*view(), *control);
            };
            break;

         case parameter::note:
            f = [this, control](double value)
            {
               // $$$ TODO: pass the actual note value $$$
               control->value(value);
               detail::refresh_element(*view(), *control);
            };
            break;
      }
      _on_parameter_change.push_back(f);

      if constexpr(sizeof...(rest) > 0)
         add_controller(id+1, std::forward<Rest>(rest)...);
   }

   template <typename... T>
   inline void controller::controls(T&&... control)
   {
      _on_parameter_change.clear();
      add_controller(0, std::forward<T>(control)...);
   }
}}

#endif