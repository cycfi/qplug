/*=============================================================================
   Copyright (c) 2016-2018 Joel de Guzman

   Distributed under the MIT License (https://opensource.org/licenses/MIT)
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <qplug/presets.hpp>

namespace q = cycfi::q;
using namespace cycfi::qplug;
using namespace q::literals;

template <typename T>
bool test_parser(preset_parser const& p, char const* in, T& attr)
{
   char const* f = in;
   char const* l = f + std::strlen(f);
   return x3::phrase_parse(f, l, p, x3::space, attr);
};

TEST_CASE("test_bool_parser")
{
   {
      bool b;
      bool r = test_parser(preset_parser{}, "true", b);
      REQUIRE(r);
      REQUIRE(b == true);
   }

   {
      bool b;
      bool r = test_parser(preset_parser{}, "false", b);
      REQUIRE(r);
      REQUIRE(b == false);
   }
}

TEST_CASE("test_int_parser")
{
   int i;
   bool r = test_parser(preset_parser{}, "123", i);
   REQUIRE(r);
   REQUIRE(i == 123);
}

TEST_CASE("test_double_parser")
{
   float f;
   bool r = test_parser(preset_parser{}, "123.456", f);
   REQUIRE(r);
   REQUIRE(f == Approx(123.456));
}

TEST_CASE("test_string_parser")
{
   std::string_view s;
   bool r = test_parser(preset_parser{}, "\"This is a string\"", s);
   REQUIRE(r);
   REQUIRE(s == "This is a string");
}

TEST_CASE("test_preset_parser")
{
   {
      parameter params[] =
      {
         parameter{ "param 1", true }
       , parameter{ "param 2", 0 }.range(0, 90)
       , parameter{ "param 3", 0.5 }
       , parameter{ "param 4", 2_kHz }
       , parameter{ "param 5", q::midi::note::E2 }
         .range(q::midi::note::A1, q::midi::note::G4)
      };

      char const* json =
      R"(
         {
            "param 1" : false,
            "param 2" : 45,
            "param 3" : 0.7,
            "param 4" : 1500,
            "param 5" : "C4"
         }
      )";

      auto&& f = [](auto const& p, parameter const& param)
      {
         switch (param._type)
         {
            case parameter::bool_:
               CHECK(p.first == "param 1");
               CHECK(p.second == false);
               break;
            case parameter::int_:
               CHECK(p.first == "param 2");
               CHECK(p.second == 45);
               break;
            case parameter::frequency:
               CHECK(p.first == "param 4");
               CHECK(p.second == 1500);
               break;
            case parameter::double_:
               CHECK(p.first == "param 3");
               CHECK(p.second == Approx(0.7));
               break;
            case parameter::note:
               CHECK(p.first == "param 5");
               CHECK(p.second == 60);
               break;
         }
      };

      auto attr = on_preset(params, f);
      bool r = test_parser(preset_parser{}, json, attr);
      REQUIRE(r);
   }
}

TEST_CASE("test_preset_parser_bad_input")
{
   {
      parameter params[] =
      {
         parameter{ "param 1", true }
       , parameter{ "param 2", 0.5 }
       , parameter{ "param 3", 2_kHz }
      };

      char const* json =
      R"(
         {
            "param 1" : false,
            "param 2" : true,    // bad type
            "param 3" : "3000"
         }
      )";

      auto&& f = [](auto const& p, parameter const& param)
      {
      };

      auto attr = on_preset(params, f);
      bool r = test_parser(preset_parser{}, json, attr);
      REQUIRE(!r);
   }
}

TEST_CASE("test_params_parser_missing_and_unordered_fields")
{
   {
      parameter params[] =
      {
         parameter{ "param 1", true }
       , parameter{ "param 2", 0 }.range(0, 90)
       , parameter{ "param 3", 0.5 }
       , parameter{ "param 4", 2_kHz }
       , parameter{ "param 5", q::midi::note::E2 }
         .range(q::midi::note::A1, q::midi::note::G4)
      };

      char const* json =
      R"(
         {
            "param 3" : 0.7,
            "param 1" : false,
            "param 4" : 1500
         }
      )";

      auto&& f = [](auto const& p, parameter const& param)
      {
         switch (param._type)
         {
            case parameter::bool_:
               CHECK(p.first == "param 1");
               CHECK(p.second == false);
               break;
            case parameter::frequency:
               CHECK(p.first == "param 4");
               CHECK(p.second == 1500);
               break;
            case parameter::double_:
               CHECK(p.first == "param 3");
               CHECK(p.second == Approx(0.7));
               break;
            default:
               CHECK(false); // We shouldn't get here
         }
      };

      auto attr = on_preset(params, f);
      bool r = test_parser(preset_parser{}, json, attr);
      REQUIRE(r);
   }
}

TEST_CASE("test_multi_presets")
{
   parameter params[] =
   {
      parameter{ "param 1", true }
    , parameter{ "param 2", 0 }.range(0, 90)
    , parameter{ "param 3", 0.5 }
    , parameter{ "param 4", 2_kHz }
    , parameter{ "param 5", q::midi::note::E2 }
      .range(q::midi::note::A1, q::midi::note::G4)
   };

   char const* json =
   R"(
      {
         "preset1" :
         {
            "param 1" : false,
            "param 2" : 45,
            "param 3" : 0.7,
            "param 4" : 1500,
            "param 5" : "C4"
         },
         "preset2" :
         {
            "param 3" : 0.7,
            "param 1" : false,
            "param 4" : 1500
         }
      }
   )";

   std::size_t i = 0;
   auto&& f1 = [&i](auto const& p, parameter const& param)
   {
      if (i == 1)
      {
         switch (param._type)
         {
            case parameter::bool_:
               CHECK(p.first == "param 1");
               CHECK(p.second == false);
               break;
            case parameter::int_:
               CHECK(p.first == "param 2");
               CHECK(p.second == 45);
               break;
            case parameter::frequency:
               CHECK(p.first == "param 4");
               CHECK(p.second == 1500);
               break;
            case parameter::double_:
               CHECK(p.first == "param 3");
               CHECK(p.second == Approx(0.7));
               break;
            case parameter::note:
               CHECK(p.first == "param 5");
               CHECK(p.second == 60);
               break;
         }
      }
      else if (i == 2)
      {
         switch (param._type)
         {
            case parameter::bool_:
               CHECK(p.first == "param 1");
               CHECK(p.second == false);
               break;
            case parameter::frequency:
               CHECK(p.first == "param 4");
               CHECK(p.second == 1500);
               break;
            case parameter::double_:
               CHECK(p.first == "param 3");
               CHECK(p.second == Approx(0.7));
               break;
            default:
               CHECK(false); // We shouldn't get here
         }
      }
   };

   auto&& f2 = [&i](std::string_view name)
   {
      if (i == 0)
         CHECK(name == "preset1");
      else if (i == 1)
         CHECK(name == "preset2");
      ++i;
   };

   auto attr = for_each_preset(params, f1, f2);
   bool r = test_parser(preset_parser{}, json, attr);
   REQUIRE(r);
}







