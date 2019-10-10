/*=============================================================================
   Copyright (c) 2016-2018 Joel de Guzman

   Distributed under the MIT License (https://opensource.org/licenses/MIT)
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <qplug/presets.hpp>

// #include <json/json.hpp>
// #include <boost/fusion/include/equal_to.hpp>
// #include <sstream>
// #include <iostream>

// namespace json = cycfi::json;
// namespace x3 = boost::spirit::x3;
// namespace fusion = boost::fusion;

using namespace cycfi::qplug;

template <typename T>
bool test_parser(parser const& p, char const* in, T& attr)
{
   char const* f = in;
   char const* l = f + std::strlen(f);
   return x3::phrase_parse(f, l, p, x3::space, attr);
};

TEST_CASE("test_parser")
{

   {
      bool b;
      bool r = test_parser(parser{}, "true", b);
      REQUIRE(r);
      REQUIRE(b == true);
   }

   {
      bool b;
      bool r = test_parser(parser{}, "false", b);
      REQUIRE(r);
      REQUIRE(b == false);
   }

   {
      int i;
      bool r = test_parser(parser{}, "123", i);
      REQUIRE(r);
      REQUIRE(i == 123);
   }

   {
      float f;
      bool r = test_parser(parser{}, "123.456", f);
      REQUIRE(r);
      REQUIRE(f == Approx(123.456));
   }

   {
      std::string_view s;
      bool r = test_parser(parser{}, "\"This is a string\"", s);
      REQUIRE(r);
      REQUIRE(s == "\"This is a string\"");
   }

   {
      std::pair<std::string_view, int> p;
      bool r = test_parser(parser{}, "\"This is a string\" : 1234", p);
      REQUIRE(r);
      REQUIRE(p.first == "\"This is a string\"");
      REQUIRE(p.second == 1234);
   }

   {
      std::pair<std::string_view, float> p;
      bool r = test_parser(parser{}, "\"This is a string\" : 123.456", p);
      REQUIRE(r);
      REQUIRE(p.first == "\"This is a string\"");
      REQUIRE(p.second == Approx(123.456));
   }
}



