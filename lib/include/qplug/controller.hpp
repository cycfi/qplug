/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_CONTROLLER_HPP_OCTOBER_17_2016)
#define QPLUG_CONTROLLER_HPP_OCTOBER_17_2016

#include <qplug/parameter.hpp>
#include <qplug/data_stream.hpp>
#include <q/support/midi.hpp>
#include <infra/iterator_range.hpp>
#include <elements/view.hpp>
#include <elements/element/button.hpp>

#include <memory>
#include <vector>
#include <map>
#include <type_traits>

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

                              controller(base_controller& base);
                              controller(controller const&) = delete;
      virtual                 ~controller();

      virtual void            on_attach_view() {}
      virtual void            on_detach_view() {}
      elements::view*         view() const;
      void                    resize_view(elements::extent size);

      using parameter_list = iterator_range<parameter const*>;

      virtual parameter_list  parameters() const = 0;

                              template <typename... T>
      void                    controls(T&&... control);

      // Call when the program wants to change parameter programmatically
      void                    set_parameter(int id, double value);
      virtual void            on_set_parameter(int id, double value) {}

      // Called when parameter is being loaded via preset
      virtual void            on_recall_parameter(int id, double value) {}

      // Called when user is editing a parameter via the GUI
      virtual void            on_edit_parameter(int id, double value) {}

      virtual void            on_parameter_change(int id, double value) {}
      virtual void            update_ui_parameter(int id, double value);

      double                  get_parameter(int id) const;
      double                  get_parameter_normalized(int id) const;
      double                  normalize_parameter(int id, double val) const;

      using preset_names_list = std::vector<std::pair<std::string_view, int>>;

      bool                    load_all_presets();
      bool                    load_preset(std::string_view name);
      std::string_view        find_preset(int program_id) const;
      int                     find_preset_id(std::string_view name) const;

      void                    save_preset(std::string_view name) const;
      bool                    delete_preset(std::string_view name);

      bool                    has_preset(int id) const;
      bool                    has_preset(std::string_view name) const;
      bool                    has_factory_preset(std::string_view name) const;
      preset_names_list       preset_list() const;

      virtual void            load_state(istream& str) {}
      virtual void            save_state(ostream& str) {}
      bool                    is_dirty() const { return _dirty; }

                              template <typename MIDIProcessor>
      void                    receive_midi(MIDIProcessor& proc);
      void                    process_midi(q::midi::raw_message msg, std::size_t time);

   private:

      friend base_controller;

      void                    recall_parameter(int id, double value);
      void                    edit_parameter(int id, double value);
      void                    parameter_change(int id, double value);

                              template <typename T, typename... Rest>
      void                    add_controller(int id, T&& first, Rest&&... rest);

      using param_change = std::function<void(double)>;
      using param_change_list = std::vector<param_change>;

      base_controller&        _base;
      param_change_list       _on_parameter_change;
      bool                    _dirty = false;

      using midi_event = std::function<void(q::midi::raw_message msg, std::size_t time)>;
      midi_event              _on_midi_event = [](auto, auto){};
   };

   using controller_ptr = std::unique_ptr<controller>;
   controller_ptr make_controller(base_controller& base);

   template <typename T>
   struct virtual_controller
    : std::enable_shared_from_this<virtual_controller<T>>
   {
                     virtual_controller(T subject)
                      : _subject(subject)
                     {}

      virtual void   set_on_change(std::function<void(double)> const& f) = 0;
      virtual void   value(double val) = 0;

      T              _subject;
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

      template <typename Element>
      inline void refresh_element(elements::view& view_, virtual_controller<std::shared_ptr<Element>>& e)
      {
         refresh_element(view_, *e._subject);
      }

      template <typename T>
      inline void refresh_element(elements::view& view_, virtual_controller<T>& e)
      {
      }
   }

   template <typename T, typename... Rest>
   inline void controller::add_controller(int id, T&& control, Rest&&... rest)
   {
      detail::set_callback(*control,
         [this, id](auto val) // Called when the user interacts with the GUI
         {
            _dirty = true;
            edit_parameter(id, val);
         }
      );

      auto const& param = parameters()[id];
      param_change f; // Called when the host wants to update the UI
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

   template <typename MIDIProcessor>
   inline void controller::receive_midi(MIDIProcessor& proc)
   {
      _on_midi_event =
         [&proc](q::midi::raw_message msg, std::size_t time)
         {
            q::midi::dispatch(msg, time, proc);
         };
   }

   inline void controller::process_midi(q::midi::raw_message msg, std::size_t time)
   {
      _on_midi_event(msg, time);
   }
}}

#endif