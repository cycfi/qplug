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
#include <elements/support/text_utils.hpp>
#include <elements/support/resource_paths.hpp>
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

      auto resources_path = dir.parent_path() / "Resources";

      // Setup the font directory as per VST3 specs:
      // https://steinbergmedia.github.io/vst3_doc/vstinterfaces/vst3loc.html
      elements::font_paths().push_back(resources_path);

      // Add the Resources to our search path
      elements::add_search_path(resources_path);
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

#endif // _WIN32

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
      _controller->update_ui_parameter(id, GetParam(id)->GetNormalized());

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
   if (view())
   {
      auto scale = view()->hdpi_scale();
      EditorResizeFromUI(size.x * scale, size.y * scale, true);
      view()->size(size);
   }
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

void iplug2_plugin::begin_edit(int id)
{
   BeginInformHostOfParamChangeFromUI(id);
}

void iplug2_plugin::edit_parameter(int id, double value)
{
   SendParameterValueFromUI(id, value);
}

void iplug2_plugin::end_edit(int id)
{
   EndInformHostOfParamChangeFromUI(id);
}

void iplug2_plugin::OnParamChange(int id, EParamSource source, int /*sampleOffset*/)
{
   if (source != kUI && _view)
      _controller->update_ui_parameter(id, GetParam(id)->GetNormalized());
   if (source == kHost)
      _controller->on_parameter_change(id, GetParam(id)->GetNormalized());
   _processor->parameter_change(id, GetParam(id)->Value());
}

// IKeyPress is used for key press info, such as ASCII representation,
// virtual key (mapped to Virtual-Key codes) and modifiers. See Virtual-Key
// Codes:
// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

// Win32 Virtual Key Codes
enum win_vk_codes
{
   vk_BackSpace = 8,
   vk_Tab = 9,
   vk_Return = 13,
   vk_Shift = 16,
   vk_Control = 17,
   vk_Alt = 18,
   vk_Pause = 19,
   vk_CapsLock = 20,
   vk_Escape = 27,
   vk_Space = 32,
   vk_PageUp = 33,
   vk_PageDown = 34,
   vk_End = 35,
   vk_Home = 36,
   vk_Left = 37,
   vk_Up = 38,
   vk_Right = 39,
   vk_Down = 40,
   vk_PrintScreen = 44,
   vk_Insert = 45,
   vk_Delete = 46,

   vk_0 = 48,
   vk_1 = 49,
   vk_2 = 50,
   vk_3 = 51,
   vk_4 = 52,
   vk_5 = 53,
   vk_6 = 54,
   vk_7 = 55,
   vk_8 = 56,
   vk_9 = 57,

   vk_A = 65,
   vk_B = 66,
   vk_C = 67,
   vk_D = 68,
   vk_E = 69,
   vk_F = 70,
   vk_G = 71,
   vk_H = 72,
   vk_I = 73,
   vk_J = 74,
   vk_K = 75,
   vk_L = 76,
   vk_M = 77,
   vk_N = 78,
   vk_O = 79,
   vk_P = 80,
   vk_Q = 81,
   vk_R = 82,
   vk_S = 83,
   vk_T = 84,
   vk_U = 85,
   vk_V = 86,
   vk_W = 87,
   vk_X = 88,
   vk_Y = 89,
   vk_Z = 90,

   vk_LWin = 91,
   vk_RWin = 92,
   vk_Apps = 93,

   vk_NumPad0 = 96,
   vk_NumPad1 = 97,
   vk_NumPad2 = 98,
   vk_NumPad3 = 99,
   vk_NumPad4 = 100,
   vk_NumPad5 = 101,
   vk_NumPad6 = 102,
   vk_NumPad7 = 103,
   vk_NumPad8 = 104,
   vk_NumPad9 = 105,

   vk_Multiply = 106,
   vk_Add = 107,
   vk_Subtract = 109,
   vk_Decimal = 110,
   vk_Divide = 111,

   vk_F1 = 112,
   vk_F2 = 113,
   vk_F3 = 114,
   vk_F4 = 115,
   vk_F5 = 116,
   vk_F6 = 117,
   vk_F7 = 118,
   vk_F8 = 119,
   vk_F9 = 120,
   vk_F10 = 121,
   vk_F11 = 122,
   vk_F12 = 123,
   vk_F13 = 124,
   vk_F14 = 125,
   vk_F15 = 126,
   vk_F16 = 127,

   vk_NumLock = 144,
   vk_ScrollLock = 145,
   vk_LShift = 160,
   vk_RShift = 161,
   vk_LControl = 162,
   vk_RControl = 163,
   vk_LAlt = 164,
   vk_RAlt = 165,
   vk_SemiColon = 186,
   vk_Equals = 187,
   vk_Comma = 188,
   vk_UnderScore = 189,
   vk_Period = 190,
   vk_Slash = 191,
   vk_BackSlash = 220,
   vk_RightBrace = 221,
   vk_LeftBrace = 219,
   vk_Apostrophe = 222
};

namespace
{
   elements::key_code translate_key(IKeyPress const& key)
   {
      using namespace elements;
      switch (key.VK)
      {
         case vk_BackSpace:               return key_code::backspace;
         case vk_Tab:                     return key_code::tab;
         case vk_Return:                  return key_code::enter;
         case vk_Shift:                   return key_code::left_shift;
         case vk_Control:                 return key_code::left_control;
         case vk_Alt:                     return key_code::left_alt;
         case vk_Pause:                   return key_code::pause;
         case vk_CapsLock:                return key_code::caps_lock;
         case vk_Escape:                  return key_code::escape;
         case vk_Space:                   return key_code::space;
         case vk_PageUp:                  return key_code::page_up;
         case vk_PageDown:                return key_code::page_down;
         case vk_End:                     return key_code::end;
         case vk_Home:                    return key_code::home;
         case vk_Left:                    return key_code::left;
         case vk_Up:                      return key_code::up;
         case vk_Right:                   return key_code::right;
         case vk_Down:                    return key_code::down;
         case vk_PrintScreen:             return key_code::print_screen;
         case vk_Insert:                  return key_code::insert;
         case vk_Delete:                  return key_code::_delete;

         case vk_0:                       return key_code:: _0;
         case vk_1:                       return key_code:: _1;
         case vk_2:                       return key_code:: _2;
         case vk_3:                       return key_code:: _3;
         case vk_4:                       return key_code:: _4;
         case vk_5:                       return key_code:: _5;
         case vk_6:                       return key_code:: _6;
         case vk_7:                       return key_code:: _7;
         case vk_8:                       return key_code:: _8;
         case vk_9:                       return key_code:: _9;

         case vk_A:                       return key_code::a;
         case vk_B:                       return key_code::b;
         case vk_C:                       return key_code::c;
         case vk_D:                       return key_code::d;
         case vk_E:                       return key_code::e;
         case vk_F:                       return key_code::f;
         case vk_G:                       return key_code::g;
         case vk_H:                       return key_code::h;
         case vk_I:                       return key_code::i;
         case vk_J:                       return key_code::j;
         case vk_K:                       return key_code::k;
         case vk_L:                       return key_code::l;
         case vk_M:                       return key_code::m;
         case vk_N:                       return key_code::n;
         case vk_O:                       return key_code::o;
         case vk_P:                       return key_code::p;
         case vk_Q:                       return key_code::q;
         case vk_R:                       return key_code::r;
         case vk_S:                       return key_code::s;
         case vk_T:                       return key_code::t;
         case vk_U:                       return key_code::u;
         case vk_V:                       return key_code::v;
         case vk_W:                       return key_code::w;
         case vk_X:                       return key_code::x;
         case vk_Y:                       return key_code::y;
         case vk_Z:                       return key_code::z;

         case vk_LWin:                    return key_code::left_super;
         case vk_RWin:                    return key_code::right_super;

         case vk_NumPad0:                 return key_code::kp_0;
         case vk_NumPad1:                 return key_code::kp_1;
         case vk_NumPad2:                 return key_code::kp_2;
         case vk_NumPad3:                 return key_code::kp_3;
         case vk_NumPad4:                 return key_code::kp_4;
         case vk_NumPad5:                 return key_code::kp_5;
         case vk_NumPad6:                 return key_code::kp_6;
         case vk_NumPad7:                 return key_code::kp_7;
         case vk_NumPad8:                 return key_code::kp_8;
         case vk_NumPad9:                 return key_code::kp_9;

         case vk_Multiply:                return key_code::_8;
         case vk_Add:                     return key_code::equal;
         case vk_Subtract:                return key_code::minus;
         case vk_Decimal:                 return key_code::period;
         case vk_Divide:                  return key_code::slash;

         case vk_F1:                      return key_code::f1;
         case vk_F2:                      return key_code::f2;
         case vk_F3:                      return key_code::f3;
         case vk_F4:                      return key_code::f4;
         case vk_F5:                      return key_code::f5;
         case vk_F6:                      return key_code::f6;
         case vk_F7:                      return key_code::f7;
         case vk_F8:                      return key_code::f8;
         case vk_F9:                      return key_code::f9;
         case vk_F10:                     return key_code::f10;
         case vk_F11:                     return key_code::f11;
         case vk_F12:                     return key_code::f12;
         case vk_F13:                     return key_code::f13;
         case vk_F14:                     return key_code::f14;
         case vk_F15:                     return key_code::f15;
         case vk_F16:                     return key_code::f16;

         case vk_NumLock:                 return key_code::num_lock;
         case vk_ScrollLock:              return key_code::scroll_lock;
         case vk_LShift:                  return key_code::left_shift;
         case vk_RShift:                  return key_code::right_shift;
         case vk_LControl:                return key_code::left_control;
         case vk_RControl:                return key_code::right_control;
         case vk_LAlt:                    return key_code::left_alt;
         case vk_RAlt:                    return key_code::right_alt;
         case vk_SemiColon:               return key_code::semicolon;
         case vk_Equals:                  return key_code::equal;
         case vk_Comma:                   return key_code::comma;
         case vk_UnderScore:              return key_code::minus;
         case vk_Period:                  return key_code::period;
         case vk_Slash:                   return key_code::slash;
         case vk_BackSlash:               return key_code::backslash;
         case vk_RightBrace:              return key_code::left_bracket;
         case vk_LeftBrace:               return key_code::right_bracket;
         case vk_Apostrophe:              return key_code::apostrophe;
      }
      return key_code::unknown;
   }

   using key_code = elements::key_code;
   using key_action = elements::key_action;
   using key_map = std::map<key_code, key_action>;

   bool handle_key(elements::base_view& _view, key_map& keys, elements::key_info k)
   {
      bool repeated = false;

      if (k.action == key_action::release
         && keys[k.key] == key_action::release)
         return false;

      if (k.action == key_action::press
         && keys[k.key] == key_action::press)
         repeated = true;

      keys[k.key] = k.action;

      if (repeated)
         k.action = key_action::repeat;

      return _view.key(k);
   }

   int get_mods(IKeyPress const& key)
   {
      int mods = 0;
      if (key.S)
         mods |= elements::mod_shift;
      if (key.C)
#if defined(__APPLE__)
         mods |= elements::mod_command | elements::mod_action;
#else
         mods |= mod_control | mod_action;
#endif
      if (key.A)
         mods |= elements::mod_alt;
      return mods;
   }
}

bool iplug2_plugin::OnKeyDown(IKeyPress const& key)
{
   if (_view)
   {
      auto code = translate_key(key);
      if (code == elements::key_code::unknown)
         return false;

      elements::key_info k = {
         code
         , key_action::press
         , get_mods(key)
      };
      bool handled = handle_key(*_view, _keys, k);

      if (key.utf8[0])
      {
         auto utf8 = &key.utf8[0];
         auto codepoint = elements::codepoint(utf8);
         int  modifiers = get_mods(key);
         return _view->text({ codepoint, modifiers }) || handled;
      }
      return handled;
   }
   return false;
}

bool iplug2_plugin::OnKeyUp(const IKeyPress& key)
{
   if (_view)
   {
      auto code = translate_key(key);
      if (code == elements::key_code::unknown)
         return false;

      elements::key_info k = {
         code
         , key_action::release
         , get_mods(key)
      };
      return handle_key(*_view, _keys, k);
   }
   return false;
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

