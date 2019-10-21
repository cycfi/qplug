/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <qplug/controller.hpp>
#include <qplug/presets.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <mutex>
#include <cstdlib>

#if defined(IPLUG2)
# include "iplug2/iplug2_plugin.hpp"
#endif

namespace cycfi { namespace qplug
{
   using preset_info = std::map<std::string, double>;
   using preset_info_map = std::map<std::string, preset_info>;
   namespace fs = boost::filesystem;

#if defined(__APPLE__)
   fs::path home = getenv("HOME");
   fs::path preset_path = home/"Library/Audio/Presets"/PLUG_MFR;
   fs::path preset_file = preset_path/PLUG_NAME"_program.json";
#endif

   // Our presets:
   preset_info_map   _presets;
   std::mutex        _presets_mutex;

   bool load_all_presets(controller::parameter_list params)
   {
      if (!fs::exists(preset_path))
         fs::create_directory(preset_path);

      if (!fs::exists(preset_file))
         return false;

      fs::ifstream file(preset_file);
      std::string src(
         (std::istreambuf_iterator<char>(file))
       , std::istreambuf_iterator<char>());

      char const* f = src.data();
      char const* l = f + src.size();

      preset_info_map loading_presets;
      std::string current_preset;

      auto&& on_param =
         [&loading_presets, &current_preset](auto const& p, parameter const& param)
         {
            std::string key(p.first.begin(), p.first.end());
            loading_presets[current_preset][key] = p.second;
         };

      auto&& on_preset_name =
         [&current_preset](std::string_view name)
         {
            current_preset = std::string(name.begin(), name.end());
         };

      auto attr = for_each_preset(params, on_param, on_preset_name);
      if (x3::phrase_parse(f, l, preset_parser{}, x3::space, attr))
      {
         std::lock_guard<std::mutex> lock(_presets_mutex);
         _presets.swap(loading_presets);
         return true;
      }
      return false;
   }

   void save_all_presets(controller::parameter_list params)
   {
      if (!fs::exists(preset_path))
         fs::create_directory(preset_path);

      fs::ofstream file(preset_file);

      std::lock_guard<std::mutex> lock(_presets_mutex);

      file << '{';
      int i = 0;
      for (auto const& [name, program] : _presets)
      {
         file << ((i++ == 0)? "\n" : ",\n");
         file << "  \"" << name << "\" : {";
         int j = 0;
         for (auto const& param : params)
         {
            // Continue if we do not have such a field
            auto name = param._name;
            auto iter = program.find(name);
            if (iter == program.end())
               continue;

            file << ((j++ == 0)? "\n" : ",\n");
            file << "    \"" << name << "\" : ";

            auto val = iter->second;
            param.print(file, val);
         }
         file << "\n  }";
      }
      file << "\n}\n";
   }

   controller::controller(base_controller& base)
      : _base(base)
   {}

   controller::~controller()
   {}

   elements::view* controller::view() const
   {
      return _base.view();
   }

   void controller::edit_parameter(int id, double value, bool notify_self)
   {
      _base.edit_parameter(id, value);
      if (notify_self)
         on_edit_parameter(id, value);
   }

   void controller::update_ui_parameter(int id, double value)
   {
      if (id < _on_parameter_change.size())
         _on_parameter_change[id](value);
   }

   void controller::parameter_change(int id, double value)
   {
      update_ui_parameter(id, value);
      on_parameter_change(id, value);
   }

   double controller::get_parameter(int id) const
   {
      return _base.get_parameter(id);
   }

   double controller::get_parameter_normalized(int id) const
   {
      return _base.get_parameter_normalized(id);
   }

   double controller::normalize_parameter(int id, double val) const
   {
      return _base.normalize_parameter(id, val);
   }

   bool controller::load_all_presets()
   {
      return qplug::load_all_presets(parameters());
   }

   bool controller::load_preset(std::string_view name)
   {
      if (name == "Default")
      {
         int i = 0;
         for (auto const& param : parameters())
         {
            auto val = normalize_parameter(i, param._init);
            edit_parameter(i++, val, true);
         }
         return true;
      }

      std::lock_guard<std::mutex> lock(_presets_mutex);
      auto preset_iter = _presets.find(std::string(name.begin(), name.end()));
      if (preset_iter != _presets.end())
      {
         int i = 0;
         auto& preset = preset_iter->second;
         for (auto const& param : parameters())
         {
            auto param_iter = preset.find(param._name);
            if (param_iter != preset.end())
            {
               auto val = normalize_parameter(i, param_iter->second);
               edit_parameter(i, val, true);
            }
            ++i;
         }
         return true;
      }
      return false;
   }

   void controller::save_preset(std::string_view name) const
   {
      {
         std::lock_guard<std::mutex> lock(_presets_mutex);
         auto &preset = _presets[std::string(name.begin(), name.end())];
         int i = 0;
         for (auto const &param : parameters())
         {
            // Skip if we do not want to save this param
            if (!param._save)
               continue;

            if (param._type == parameter::note)
            {
               auto range =  param._max - param._min;
               preset[param._name] =
                  (get_parameter_normalized(i++) * range) + param._min
               ;
            }
            else
            {
               preset[param._name] = get_parameter(i++);
            }
         }
      }
      save_all_presets(parameters());
   }

   bool controller::has_preset(std::string_view name) const
   {
      std::lock_guard<std::mutex> lock(_presets_mutex);
      auto i = _presets.find(std::string(name.begin(), name.end()));
      return i != _presets.end();
   }

   controller::preset_names_list controller::preset_list() const
   {
      std::lock_guard<std::mutex> lock(_presets_mutex);
      preset_names_list r;
      for (auto const& [name, program] : _presets)
         r.push_back(name);
      return r;
   }
}}
