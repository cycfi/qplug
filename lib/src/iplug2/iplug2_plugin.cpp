/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "iplug2_plugin.hpp"
#include "IPlug_include_in_plug_src.h"

namespace elements = cycfi::elements;
namespace q = cycfi::q;

iplug2_plugin::iplug2_plugin(InstanceInfo const& info)
  : iplug2_plugin(info, qplug::make_controller(*this))
{}

iplug2_plugin::iplug2_plugin(InstanceInfo const& info, controller_ptr&& cptr)
  : Plugin(info, MakeConfig(cptr->parameters().size(), kNumPrograms))
  , _controller(std::forward<controller_ptr>(cptr))
  , _processor(qplug::make_processor(*this))
{
   auto params = _controller->parameters();
   for (std::size_t i = 0; i != params.size(); ++i)
      register_parameter(i, params[i]);
}

void iplug2_plugin::ProcessBlock(sample** inputs, sample** outputs, int frames)
{
   _processor->process(
      q::audio_channels<float const>{
         const_cast<float const**>(inputs)
       , std::size_t(NInChansConnected())
       , std::size_t(frames)
      }
      , q::audio_channels<float>{
         outputs
       , std::size_t(NOutChansConnected())
       , std::size_t(frames)
      }
   );
}

void* iplug2_plugin::OpenWindow(void* parent)
{
   if (parent)
      _view = std::make_unique<elements::view>(static_cast<elements::host_view_handle>(parent));
   else
      _view = std::make_unique<elements::view>(elements::extent{ PLUG_WIDTH, PLUG_HEIGHT });

  _controller->on_attach_view();

   for (int id = 0; id != NParams(); ++id)
      _controller->parameter_change(id, GetParam(id)->GetNormalized());

   _view->refresh();
   return _view->host();
}

void iplug2_plugin::CloseWindow()
{
  _controller->on_detach_view();
   _view.reset();
}

void iplug2_plugin::OnReset()
{
   _processor->reset();
}

void iplug2_plugin::OnActivate(bool active)
{
   if (active)
      _processor->activate();
   else
      _processor->deactivate();
}

void iplug2_plugin::register_parameter(int id, qplug::parameter const& param)
{
   constexpr auto kUnitCustom = iplug::IParam::kUnitCustom;
   constexpr auto kFlagCannotAutomate = iplug::IParam::EFlags::kFlagCannotAutomate;

   using Shape = iplug::IParam::Shape;
   using ShapeLinear = iplug::IParam::ShapeLinear;
   using ShapePowCurve = iplug::IParam::ShapePowCurve;
   int flags = param._can_automate? 0 : kFlagCannotAutomate;

   switch (param._type)
   {
      case qplug::parameter::bool_:
         GetParam(id)->InitBool(
            param._name
          , param._init > 0.5
          , param._unit
          , flags          // flags
          , ""             // group
         );
         break;

      case qplug::parameter::int_:
         GetParam(id)->InitInt(
            param._name
          , param._init
          , param._min
          , param._max
          , param._unit
          , flags          // flags
          , ""             // group
         );
         break;

      case qplug::parameter::double_:
      {
         auto linear = ShapeLinear();
         auto power = ShapePowCurve(param._curve);
         Shape&& shape = (param._curve == 1.0)?
            std::forward<Shape>(linear) :
            std::forward<Shape>(power);

         GetParam(id)->InitDouble(
            param._name
          , param._init
          , param._min
          , param._max
          , param._step
          , param._unit
          , flags          // flags
          , ""             // group
          , shape          // shape
          , kUnitCustom    // unit
          , nullptr        // displayFunc
         );
      }
      break;
   }
}

void iplug2_plugin::edit_parameter(int id, double value)
{
   SendParameterValueFromUI(id, value);
}

void iplug2_plugin::OnParamChange(int id, EParamSource source, int sampleOffset)
{
   if (source != kUI && _view)
      _controller->parameter_change(id, GetParam(id)->GetNormalized());
   _processor->parameter_change(id, GetParam(id)->Value());
}

