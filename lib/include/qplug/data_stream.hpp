/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_DATA_STREAM_OCTOBER_21_2019)
#define QPLUG_DATA_STREAM_OCTOBER_21_2019

#include <infra/data_stream.hpp>

namespace cycfi::qplug
{
   struct ostream : cycfi::ostream<ostream>
   {
      virtual ostream& write(const char* s, std::size_t size) = 0;
   };

   struct istream : cycfi::istream<istream>
   {
      virtual char const* data() const = 0;
      virtual std::size_t size() const = 0;
   };
}

#endif