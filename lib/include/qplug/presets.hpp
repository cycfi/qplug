/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_PRESETS_HPP_OCTOBER_10_2019)
#define QPLUG_PRESETS_HPP_OCTOBER_10_2019

#include <qplug/parameter.hpp>
#include <infra/iterator_range.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_pair.hpp>

#include <map>
#include <functional>
#include <string_view>

namespace cycfi { namespace qplug
{
   class preset
   {
   public:

      using parameter_list = iterator_range<parameter const*>;

                        preset(
                           parameter_list* param_list
                         , std::string_view json
                        )
                         : _param_list(param_list)
                         , _json(json)
                        {}

                        template <typename F>
      void              for_each(F&& f);

   private:

      parameter_list*   _param_list;
      std::string       _json;
   };

   namespace x3 = boost::spirit::x3;
   namespace fusion = boost::fusion;

   struct malformed_json_string
    : std::runtime_error { using runtime_error::runtime_error; };

   class parser : public x3::parser<parser>
   {
   public:

      struct attribute {};
      typedef attribute attribute_type;
      static bool const has_attribute = true;

      // Main parse entry point
      template <typename Iter, typename Ctx, typename Attr>
      bool parse(Iter& first, Iter const& last
       , Ctx& context, x3::unused_type, Attr& attr) const
      {
         return parse_impl(first, last, context, attr);
      }

   private:

      parser const& self() const { return *this; }

      // Floating point numbers
      template <typename Iter, typename Ctx, typename Attr>
      typename std::enable_if<std::is_floating_point<Attr>::value, bool>::type
      parse_impl(Iter& first, Iter last, Ctx& context, Attr& attr) const;

      // Integers
      template <typename Iter, typename Ctx, typename Attr>
      typename std::enable_if<std::is_integral<Attr>::value, bool>::type
      parse_impl(Iter& first, Iter last, Ctx& context, Attr& attr) const;

      // Boolean
      template <typename Iter, typename Ctx>
      bool parse_impl(Iter& first, Iter last, Ctx& context, bool& attr) const;

      // String View
      template <typename Iter, typename Ctx>
      bool parse_impl(Iter& first, Iter last, Ctx& context, std::string_view& attr) const;

      // Pair
      template <typename Iter, typename Ctx, typename Attr>
      bool parse_impl(Iter& first, Iter last, Ctx& context, std::pair<std::string_view, Attr>& attr) const;
   };

///////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////
   template <typename Iter, typename Ctx, typename Attr>
   inline typename std::enable_if<std::is_floating_point<Attr>::value, bool>::type
   parser::parse_impl(Iter& first, Iter last, Ctx& context, Attr& attr) const
   {
      static x3::real_parser<Attr> p;
      return p.parse(first, last, context, x3::unused, attr);
   }

   template <typename Iter, typename Ctx, typename Attr>
   inline typename std::enable_if<std::is_integral<Attr>::value, bool>::type
   parser::parse_impl(Iter& first, Iter last, Ctx& context, Attr& attr) const
   {
      static x3::int_parser<Attr> p;
      return p.parse(first, last, context, x3::unused, attr);
   }

   template <typename Iter, typename Ctx>
   inline bool
   parser::parse_impl(Iter& first, Iter last, Ctx& context, bool& attr) const
   {
      return x3::bool_.parse(first, last, context, x3::unused, attr);
   }

   template <typename Iter, typename Ctx>
   inline bool
   parser::parse_impl(Iter& first, Iter last, Ctx& context, std::string_view& attr) const
   {
      x3::skip_over(first, last, context);

      // Parse this manually for speed. We do not decode nor validate anything.
      // We simply detect what looks like a double-quoted string which contains
      // any character, including any escaped double quote (i.e. "\\\""). We'll
      // deal with the actual string validation and extraction later.

      static auto double_quote = '"' >> *("\\\"" | (x3::char_ - '"')) >> '"';
      auto iter = first;
      if (double_quote.parse(iter, last, context, x3::unused, x3::unused))
      {
         attr = std::string_view{first, std::size_t(iter-first)};
         first = iter;
         return true;
      }
      return false;
   }

   // Pair
   template <typename Iter, typename Ctx, typename Attr>
   inline bool
   parser::parse_impl(Iter& first, Iter last, Ctx& context, std::pair<std::string_view, Attr>& attr) const
   {
      static auto g = parser{} >> ':' >> parser{};
      return g.parse(first, last, context, x3::unused, attr);
   }
}}

#endif