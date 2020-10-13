/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "iplug2_plugin.hpp"
#include <qplug/data_stream.hpp>
#include <infra/filesystem.hpp>
#include "IPlug_include_in_plug_src.h"
#include <sstream>
#include <elements/support/font.hpp>
#include <algorithm>

namespace elements = cycfi::elements;
namespace q = cycfi::q;

#if defined(_WIN32)

namespace fs = cycfi::fs;

struct startup
{
   startup()
   {
      // Load DLLS
      auto dir = module_dir();
      std::istringstream dll_names{ QPLUG_DLL_LINK_ORDER };
      std::for_each(
         std::istream_iterator<std::string>{dll_names}
       , std::istream_iterator<std::string>{}
       , [&](auto const& name)
         {
            auto const& path = dir / (name + ".dll");
            CYCFI_ASSERT(fs::exists(path), "Fatal Error: Missing DLL (" + path.string() + ").");
            auto module = LoadLibraryW(path.wstring().data());
            CYCFI_ASSERT(module != nullptr, "Fatal Error: Failed to load DLL (" + path.string() + ").");
         }
      );

      // Setup the font directory as per VST3 specs:
      // https://steinbergmedia.github.io/vst3_doc/vstinterfaces/vst3loc.html
      elements::font_paths().push_back(dir.parent_path() / "Resources");
   }

   static fs::path module_dir()
   {
      wchar_t path[MAX_PATH];
      HMODULE hm = nullptr;
      constexpr auto flags =
         GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
         GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
         ;

      if (GetModuleHandleExW(flags, (LPCWSTR) &startup::module_dir, &hm))
         if (GetModuleFileNameW(hm, path, sizeof(path)))
            return fs::path{ path }.parent_path();
      return {};
   }
};

auto _startup = startup{};

#endif

iplug2_plugin::iplug2_plugin(InstanceInfo const& info)
  : iplug2_plugin(info, qplug::make_controller(*this))
{}

iplug2_plugin::iplug2_plugin(InstanceInfo const& info, controller_ptr&& cptr)
  : Plugin(info, MakeConfig(cptr->parameters().size(), 1))
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

void iplug2_plugin::ProcessMidiMsg(const IMidiMsg& msg)
{
   q::midi::raw_message raw_midi = { 0 };
   raw_midi.data = msg.mStatus | (msg.mData1 << 8) | (msg.mData2 << 16);
   _controller->process_midi(raw_midi, msg.mOffset);
}

void* iplug2_plugin::OpenWindow(void* parent)
{
   if (parent)
      _view = std::make_unique<elements::view>(static_cast<elements::host_view_handle>(parent));
   else
      _view = std::make_unique<elements::view>(elements::extent{ PLUG_WIDTH, PLUG_HEIGHT });

   _controller->on_attach_view();
   _controller->load_all_presets();

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
         Shape const& shape = (param._curve == 1.0)?
            static_cast<Shape const&>(linear) : static_cast<Shape const&>(power);

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

      case qplug::parameter::note:
         GetParam(id)->InitPitch(
            param._name
          , param._init - param._min
          , param._min
          , param._max
          , flags          // flags
          , ""             // group
          , true           // middleCisC
         );
         break;

      case qplug::parameter::frequency:
         GetParam(id)->InitFrequency(
            param._name
          , param._init
          , param._min
          , param._max
          , param._step
          , flags          // flags
          , ""             // group
         );
         break;
   }
}

void iplug2_plugin::resize_view(elements::extent size)
{
   EditorResizeFromUI(size.x, size.y, true);
   if (view())
      view()->size(size);
}

void iplug2_plugin::set_parameter(int id, double value)
{
   IParam* param = mParams.Get(id);
   if (param)
   {
      param->SetNormalized(value);
      _processor->parameter_change(id, GetParam(id)->Value());
   }
}

void iplug2_plugin::recall_parameter(int id, double value)
{
   IParam* param = mParams.Get(id);
   if (param)
   {
      param->SetNormalized(value);
      OnParamChange(id, kPresetRecall);
      OnParamChangeUI(id, kPresetRecall);
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

namespace
{
   struct iplug2_ostream : qplug::ostream
   {
      iplug2_ostream(IByteChunk& chunk)
       : _chunk(chunk)
      {}

      ostream& write(const char* s, std::size_t size) override
      {
         if (_ok)
            _ok &= _chunk.PutBytes(s, size) > 0;
         return *this;
      }

      IByteChunk& _chunk;
      bool        _ok = true;
   };

   struct iplug2_istream : qplug::istream
   {
      iplug2_istream(IByteChunk const& chunk, int start_pos)
       : _chunk(chunk)
       , _start_pos(start_pos)
      {}

      char const* data() const override
      {
         return reinterpret_cast<char const*>(_chunk.GetData()) + _start_pos;
      }

      std::size_t size() const override
      {
         return _chunk.Size() - _start_pos;
      }

      IByteChunk const& _chunk;
      int               _start_pos;
   };
}

bool iplug2_plugin::SerializeState(IByteChunk& chunk) const
{
   IByteChunk::InitChunkWithIPlugVer(chunk);
   iplug2_ostream str{ chunk };

   int num_params = mParams.GetSize();
   str << num_params;
   for (int i = 0; i < num_params; ++i)
   {
      IParam* param = mParams.Get(i);
      str << param->GetName() << param->Value();
   }

   try
   {
      _controller->save_state(str);
   }
   catch (...)
   {
      return false;
   }

   return str._ok;
}

namespace
{
   IParam* get_param(std::string_view name, WDL_PtrList<IParam> const& params)
   {
      int n = params.GetSize();
      for (int i = 0; i < n; ++i)
      {
         IParam* param = params.Get(i);
         if (param && param->GetName() == name)
            return param;
      }
      return nullptr;
   }
}

int iplug2_plugin::UnserializeState(IByteChunk const& chunk, int start_pos)
{
   auto current_version = GetPluginVersion(false);
   auto version = IByteChunk::GetIPlugVerFromChunk(chunk, start_pos);

   if (version != current_version) // $$$ temporary $$$
   {
      auto pos = Plugin::UnserializeState(chunk, start_pos);
      iplug2_istream str{ chunk, pos };
      try
      {
         _controller->load_state(str);
      }
      catch (...)
      {
         return false;
      }

      return pos + str.offset();
   }
   else
   {
      iplug2_istream str{ chunk, start_pos };
      int num_params;
      str >> num_params;

      ENTER_PARAMS_MUTEX
      for (int i = 0; i < num_params; ++i)
      {
         std::string name;
         double value;
         str >> name >> value;

         auto param = get_param(name, mParams);
         if (param)
            param->Set(value);
      }
      OnParamReset(kPresetRecall);
      LEAVE_PARAMS_MUTEX

      try
      {
         _controller->load_state(str);
      }
      catch (...)
      {
         return false;
      }
   }
   return start_pos;
}

double iplug2_plugin::get_parameter(int id) const
{
   return GetParam(id)->Value();
}

double iplug2_plugin::get_parameter_normalized(int id) const
{
   return GetParam(id)->GetNormalized();
}

double iplug2_plugin::normalize_parameter(int id, double val) const
{
   return GetParam(id)->ToNormalized(val);
}

std::uint32_t iplug2_plugin::sps() const
{
   return GetSampleRate();
}

bool iplug2_plugin::bypassed() const
{
   return GetBypassed();
}

