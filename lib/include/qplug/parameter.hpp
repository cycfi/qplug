/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#pragma once
#include <type_traits>
#include <algorithm>

namespace cycfi { namespace qplug
{
   struct parameter
   {
      enum type { bool_, int_, double_ };

      template <typename T
       , typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr
      >
      constexpr parameter(char const* name, T init)
       : _name(name)
       , _type(double_)
       , _init(init)
       , _min(std::min(0.0, init))
       , _max(std::max(1.0, init))
      {}

      template <typename T
       , typename std::enable_if<std::is_integral<T>::value>::type* = nullptr
      >
      constexpr parameter(char const* name, T init)
       : _name(name)
       , _type(std::is_same<T, bool>::value ? bool_ : int_)
       , _init(init)
       , _min(std::min(0.0, init))
       , _max(std::max(1.0, init))
      {}

      template <typename T>
      constexpr parameter range(T min, T max) const
      {
         parameter r = *this;
         r._min = min;
         r._max = max;
         std::clamp(_init, double(min), double(max));
         return r;
      }

      template <typename T>
      constexpr parameter range(T min, T max, T step) const
      {
         parameter r = *this;
         r._min = min;
         r._max = max;
         r._step = step;
         std::clamp(_init, min, max);
         return r;
      }

      constexpr parameter curve(double curve_) const
      {
         parameter r = *this;
         r._curve = curve_;
         return r;
      }

      constexpr parameter dont_automate() const
      {
         parameter r = *this;
         r._can_automate = false;
         return r;
      }

      constexpr parameter unit(char const* unit_) const
      {
         parameter r = *this;
         r._unit = unit_;
         return r;
      }

      char const*    _name;
      type           _type;
      double         _init;
      double         _min = 0.0;
      double         _max = 1.0;
      double         _step = 0.001;
      double         _curve = 1.0;
      char const*    _unit = nullptr;
      bool           _can_automate = true;
   };
}}
