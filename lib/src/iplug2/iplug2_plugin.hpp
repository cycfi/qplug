/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_IPLUG2_PLUGIN_HPP_APRIL_18_2019)
#define QPLUG_IPLUG2_PLUGIN_HPP_APRIL_18_2019

#include "IPlug_include_in_plug_hdr.h"
#include <elements/view.hpp>
#include <qplug/controller.hpp>
#include <qplug/processor.hpp>
#include <qplug/parameter.hpp>
#include <memory>
#include <map>

using namespace iplug;
namespace elements = cycfi::elements;
namespace qplug = cycfi::qplug;

class iplug2_plugin : public Plugin
{
public:

   using view_ptr = std::unique_ptr<elements::view>;
   using controller_ptr = std::unique_ptr<qplug::controller>;
   using processor_ptr = std::unique_ptr<qplug::processor>;

                           iplug2_plugin(InstanceInfo const& info);
                           iplug2_plugin(InstanceInfo const& info, controller_ptr&& cptr);

   void*                   OpenWindow(void* parent) override;
   void                    CloseWindow() override;
   void                    OnReset() override;
   void                    OnActivate(bool active) override;
   void                    ProcessBlock(sample** in, sample** out, int frames) override;
   void                    ProcessMidiMsg(const IMidiMsg& msg) override;
   void                    OnParamChange(int id, EParamSource source, int sampleOffset = -1) override;

#if defined(VST3_API)
   bool                    OnKeyDown(IKeyPress const& key) override;
   bool                    OnKeyUp(IKeyPress const& key) override;
#endif

   bool                    SerializeState(IByteChunk& chunk) const override;
   int                     UnserializeState(IByteChunk const& chunk, int start_pos) override;

   elements::view*         view() const { return _view.get(); }
   void                    resize_view(elements::extent size);

   void                    set_parameter(int id, double value);
   void                    recall_parameter(int id, double value);
   void                    begin_edit(int id);
   void                    edit_parameter(int id, double value);
   void                    end_edit(int id);
   double                  normalize_parameter(int id, double val) const;
   double                  get_parameter(int id) const;
   double                  get_parameter_normalized(int id) const;

   std::uint32_t           sps() const;
   bool                    bypassed() const;

private:

   using key_code = elements::key_code;
   using key_action = elements::key_action;
   using key_map = std::map<key_code, key_action>;

   void                    register_parameter(int id, qplug::parameter const& param);

   view_ptr                _view;
   controller_ptr          _controller;
   processor_ptr           _processor;
   key_map                 _keys = {};
};

#endif