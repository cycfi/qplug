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

#if defined(VST3_API)

// IKeyPress is used for key press info, such as ASCII representation,
// virtual key (mapped to Virtual-Key codes) and modifiers. See Virtual-Key
// Codes: https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
// Take note that not all virtual-Key codes are reported back, limited by the
// switch below.

namespace
{
   elements::key_code translate_key(IKeyPress const& key)
   {
      using namespace elements;

      if (key.S)
         return key_code::left_shift;
      if (key.C)
         return key_code::left_super;
      if (key.A)
         return key_code::left_alt;

      switch (key.VK)
      {
         case kVK_NONE:                         return key_code::unknown;
         case kVK_LBUTTON:                      return key_code::unknown;
         case kVK_RBUTTON:                      return key_code::unknown;
         case kVK_MBUTTON:                      return key_code::unknown;

         case kVK_BACK:                         return key_code::backspace;
         case kVK_TAB:                          return key_code::tab;

         case kVK_CLEAR:      /*$$$ fixme $$$*/ return key_code::unknown;
         case kVK_RETURN:                       return key_code::enter;

         case kVK_SHIFT:                        return key_code::left_shift;
         case kVK_CONTROL:                      return key_code::left_control;
         case kVK_MENU:                         return key_code::menu;
         case kVK_PAUSE:                        return key_code::pause;
         case kVK_CAPITAL:                      key_code::caps_lock;

         case kVK_ESCAPE:                       return key_code::escape;

         case kVK_SPACE:                        return key_code::space;
         case kVK_PRIOR:                        return key_code::page_up;
         case kVK_NEXT:                         return key_code::page_down;
         case kVK_END:                          return key_code::end;
         case kVK_HOME:                         return key_code::home;
         case kVK_LEFT:                         return key_code::left;
         case kVK_UP:                           return key_code::up;
         case kVK_RIGHT:                        return key_code::right;
         case kVK_DOWN:                         return key_code::down;

#if !defined(__APPLE__) // On the Mac, we will not send these:
                        // We'll just let element's base_view to handle them.

         case kVK_SELECT:     /*$$$ fixme $$$*/ return key_code::unknown;
         case kVK_PRINT:      /*$$$ fixme $$$*/ return key_code::unknown;
         case kVK_SNAPSHOT:                     return key_code::print_screen;
         case kVK_INSERT:                       return key_code::insert;
         case kVK_DELETE:                       return key_code::_delete;
         case kVK_HELP:       /*$$$ fixme $$$*/ return key_code::unknown;

#endif // __APPLE__

         case kVK_0:                            return key_code:: _0;
         case kVK_1:                            return key_code:: _1;
         case kVK_2:                            return key_code:: _2;
         case kVK_3:                            return key_code:: _3;
         case kVK_4:                            return key_code:: _4;
         case kVK_5:                            return key_code:: _5;
         case kVK_6:                            return key_code:: _6;
         case kVK_7:                            return key_code:: _7;
         case kVK_8:                            return key_code:: _8;
         case kVK_9:                            return key_code:: _9;

         case kVK_A:                            return key_code::a;
         case kVK_B:                            return key_code::b;
         case kVK_C:                            return key_code::c;
         case kVK_D:                            return key_code::d;
         case kVK_E:                            return key_code::e;
         case kVK_F:                            return key_code::f;
         case kVK_G:                            return key_code::g;
         case kVK_H:                            return key_code::h;
         case kVK_I:                            return key_code::i;
         case kVK_J:                            return key_code::j;
         case kVK_K:                            return key_code::k;
         case kVK_L:                            return key_code::l;
         case kVK_M:                            return key_code::m;
         case kVK_N:                            return key_code::n;
         case kVK_O:                            return key_code::o;
         case kVK_P:                            return key_code::p;
         case kVK_Q:                            return key_code::q;
         case kVK_R:                            return key_code::r;
         case kVK_S:                            return key_code::s;
         case kVK_T:                            return key_code::t;
         case kVK_U:                            return key_code::u;
         case kVK_V:                            return key_code::v;
         case kVK_W:                            return key_code::w;
         case kVK_X:                            return key_code::x;
         case kVK_Y:                            return key_code::y;
         case kVK_Z:                            return key_code::z;

         case kVK_LWIN:                         return key_code::left_super;

         case kVK_NUMPAD0:                      return key_code::kp_0;
         case kVK_NUMPAD1:                      return key_code::kp_1;
         case kVK_NUMPAD2:                      return key_code::kp_2;
         case kVK_NUMPAD3:                      return key_code::kp_3;
         case kVK_NUMPAD4:                      return key_code::kp_4;
         case kVK_NUMPAD5:                      return key_code::kp_5;
         case kVK_NUMPAD6:                      return key_code::kp_6;
         case kVK_NUMPAD7:                      return key_code::kp_7;
         case kVK_NUMPAD8:                      return key_code::kp_8;
         case kVK_NUMPAD9:                      return key_code::kp_9;

         case kVK_MULTIPLY:     /*$$$ ??? $$$*/ return key_code::_8;
         case kVK_ADD:          /*$$$ ??? $$$*/ return key_code::equal;
         case kVK_SEPARATOR:                    return key_code::backslash;
         case kVK_SUBTRACT:     /*$$$ ??? $$$*/ return key_code::minus;
         case kVK_DECIMAL:                      return key_code::period;
         case kVK_DIVIDE:                       return key_code::slash;

         case kVK_F1:                           return key_code::f1;
         case kVK_F2:                           return key_code::f2;
         case kVK_F3:                           return key_code::f3;
         case kVK_F4:                           return key_code::f4;
         case kVK_F5:                           return key_code::f5;
         case kVK_F6:                           return key_code::f6;
         case kVK_F7:                           return key_code::f7;
         case kVK_F8:                           return key_code::f8;
         case kVK_F9:                           return key_code::f9;
         case kVK_F10:                          return key_code::f10;
         case kVK_F11:                          return key_code::f11;
         case kVK_F12:                          return key_code::f12;
         case kVK_F13:                          return key_code::f13;
         case kVK_F14:                          return key_code::f14;
         case kVK_F15:                          return key_code::f15;
         case kVK_F16:                          return key_code::f16;
         case kVK_F17:                          return key_code::f17;
         case kVK_F18:                          return key_code::f18;
         case kVK_F19:                          return key_code::f19;
         case kVK_F20:                          return key_code::f20;
         case kVK_F21:                          return key_code::f21;
         case kVK_F22:                          return key_code::f22;
         case kVK_F23:                          return key_code::f23;
         case kVK_F24:                          return key_code::f24;

         case kVK_NUMLOCK:                      return key_code::num_lock;
         case kVK_SCROLL:                       return key_code::scroll_lock;
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
         mods |= elements::mod_control | elements::mod_action;
#endif
      if (key.A)
         mods |= elements::mod_alt;
      return mods;
   }

   uint32_t get_codepoint(IKeyPress const& key)
   {
#if defined(_WIN32)

      // Win32 version of the VST3 SDK gives wrong results with key.utf8 depending on host.
      // We do our extraction here, ignorng the information from the VST3 API.

      BYTE kb[256];
      for (auto i = 0; i < 256; i++)
         kb[i] = GetKeyState(i);
      WCHAR uc[12] = {};
      auto r = ToUnicode(key.VK, MapVirtualKey(key.VK, MAPVK_VK_TO_VSC), kb, uc, 12, 0);

      switch (r)
      {
         case -1: // dead key
         case  0: // no translation
            return 0;

         default:
            int size = WideCharToMultiByte(CP_UTF8, 0, uc, 12, nullptr, 0, nullptr, nullptr);
            std::string utf8(size, 0);
            WideCharToMultiByte(CP_UTF8, 0, uc, 12, utf8.data(), size, nullptr, nullptr);
            char const* p = utf8.data();
            // $$$ JDG: is there a chance that we'll get more than 1 codepoint here? $$$
            return elements::codepoint(p);
      }
      return 0;

#else
      if (key.utf8[0] == 0)
         return 0;
      auto utf8 = &key.utf8[0];
      auto cp = elements::codepoint(utf8);
      return (utf8 != &key.utf8[0])? cp : 0;
#endif
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

      if (!handled)
      {
         auto cp = get_codepoint(key);
         if (cp)
         {
            if (cp < 32 || (cp > 126 && cp < 160))
               return false;
            int  modifiers = get_mods(key);
            return _view->text({ cp, modifiers });
         }
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

#endif // defined(VST3_API)

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

