/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <qplug/controller.hpp>
#include <qplug/presets.hpp>
#include <infra/filesystem.hpp>
#include <elements/support/resource_paths.hpp>

#include <mutex>
#include <cstdlib>
#include <fstream>

#if defined(IPLUG2)
# include "iplug2/iplug2_plugin.hpp"
#endif

namespace cycfi { namespace qplug
{
   using preset_info = std::map<std::string, double>;
   using preset_info_map = std::map<std::string, preset_info>;

#if defined(__APPLE__)
   fs::path home = getenv("HOME");
   fs::path presets_path = home / "Library/Audio/Presets" / PLUG_MFR;
   fs::path presets_file = presets_path / PLUG_NAME"_presets.json";
#endif

   // Factory presets:
   preset_info_map   _factory_presets;
   std::mutex        _factory_presets_mutex;

   // User presets:
   preset_info_map   _presets;
   std::mutex        _presets_mutex;

   bool load_all_presets(
      std::string const& src
    , controller::parameter_list params
    , preset_info_map& presets)
   {
      char const* f = src.data();
      char const* l = f + src.size();
      std::string current_preset;

      auto&& on_param =
         [&presets, &current_preset](auto const& p, parameter const& param)
         {
            std::string key(p.first.begin(), p.first.end());
            presets[current_preset][key] = p.second;
         };

      auto&& on_preset_name =
         [&current_preset](std::string_view name)
         {
            current_preset = std::string(name.begin(), name.end());
         };

      auto attr = for_each_preset(params, on_param, on_preset_name);
      if (x3::phrase_parse(f, l, preset_parser{}, x3::space, attr))
         return true;
      return false;
   }

   bool load_all_presets(
      fs::path const& preset_file
    , controller::parameter_list params
    , preset_info_map& presets)
   {
      if (!fs::exists(preset_file))
         return false;

      std::ifstream file(preset_file);
      std::string src(
         (std::istreambuf_iterator<char>(file))
       , std::istreambuf_iterator<char>());

      return load_all_presets(src, params, presets);
   }

   bool load_all_presets(controller::parameter_list params)
   {
      if (!fs::exists(presets_path))
         fs::create_directory(presets_path);

      // Load factory presets
      if (_factory_presets.empty())
      {
         fs::path full_path = elements::find_file("factory_presets.json");
         preset_info_map loading_presets;
         if (load_all_presets(full_path, params, loading_presets))
         {
            std::lock_guard<std::mutex> lock(_factory_presets_mutex);
            _factory_presets.swap(loading_presets);
         }
      }

      // Load user presets
      if (_presets.empty())
      {
         preset_info_map loading_presets;
         if (load_all_presets(presets_file, params, loading_presets))
         {
            std::lock_guard<std::mutex> lock(_presets_mutex);
            _presets.swap(loading_presets);
         }
      }
      return true;
   }

   void save_all_presets(controller::parameter_list params)
   {
      if (!fs::exists(presets_path))
         fs::create_directory(presets_path);

      std::ofstream file(presets_file);

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

   void controller::resize_view(elements::extent size)
   {
      _base.resize_view(size);
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

      std::lock_guard<std::mutex> lock1(_presets_mutex);
      std::lock_guard<std::mutex> lock2(_factory_presets_mutex);

      auto preset_iter = _presets.find(std::string(name.begin(), name.end()));
      auto preset_iter_end =_presets.end();

      if (preset_iter == _presets.end())
      {
         preset_iter = _factory_presets.find(std::string(name.begin(), name.end()));
         preset_iter_end = _factory_presets.end();
      }

      if (preset_iter != preset_iter_end)
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

   std::string_view controller::find_program_id(int program_id) const
   {
      auto&& find_preset = [program_id](auto const& presets) -> std::string_view
      {
         for (auto const& [name, program] : presets)
         {
            auto iter = program.find("Program ID");
            if (iter != program.end() && iter->second == program_id)
               return name;
         }
         return "";
      };

      auto r = find_preset(_factory_presets);
      return r.empty()? find_preset(_presets) : r;
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
            {
               ++i;
               continue;
            }

            // Special case for Program IDs
            if (std::strcmp(param._name, "Program ID") == 0)
            {
               // See if there's a conflict of IDs
               int pc = get_parameter(i);
               auto pc_owner = find_program_id(pc);
               if (pc_owner != "" && pc_owner != name)
               {
                  // If there's a conflict, assign the owner_preset's ID with
                  // the preset's old ID
                  auto& owner_preset = _presets[std::string(pc_owner.begin(), pc_owner.end())];
                  owner_preset[param._name] = preset[param._name];
               }
            }

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

   bool controller::delete_preset(std::string_view name)
   {
      bool proceed = false;
      {
         std::lock_guard<std::mutex> lock(_presets_mutex);
         auto preset_iter = _presets.find(std::string(name.begin(), name.end()));
         if (preset_iter != _presets.end())
         {
            _presets.erase(preset_iter);
            proceed = true;
         }
      }
      if (proceed)
         save_all_presets(parameters());
      return proceed;
   }

   bool controller::has_preset(std::string_view name) const
   {
      // Search the factory presets
      if (has_factory_preset(name))
         return true;

      // Search the user presets
      std::string preset_name{ name.begin(), name.end() };
      std::lock_guard<std::mutex> lock(_presets_mutex);
      auto i = _presets.find(preset_name);
      return i != _presets.end();
   }

   bool controller::has_factory_preset(std::string_view name) const
   {
      // Search the factory presets
      std::string preset_name{ name.begin(), name.end() };
      std::lock_guard<std::mutex> lock(_factory_presets_mutex);
      auto i = _factory_presets.find(preset_name);
      return i != _factory_presets.end();
   }

   controller::preset_names_list controller::preset_list() const
   {
      std::map<int, std::string_view> rmap;
      preset_names_list r;

      auto&& add =
         [&rmap, &r](auto const& name, auto const& program)
         {
            // If there's a "Program ID", we store it in the
            // result sorted by the ID
            auto iter = program.find("Program ID");
            if (iter != program.end())
               rmap[iter->second] = name;
            else
               r.push_back(name);
         };

      // Get the factory presets
      {
         std::lock_guard<std::mutex> lock(_factory_presets_mutex);
         for (auto const& [name, program] : _factory_presets)
            add(name, program);
      }

      // Get the user presets
      {
         std::lock_guard<std::mutex> lock(_presets_mutex);
         for (auto const& [name, program] : _presets)
            add(name, program);
      }

      for (auto const& [id, name] : rmap)
         r.push_back(name);
      return r;
   }
}}
