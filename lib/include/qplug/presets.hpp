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
#include <optional>

namespace cycfi { namespace qplug
{
   class preset
   {
   public:

      using parameter_list = iterator_range<parameter const*>;

                        preset(
                           parameter_list param_list
                         , std::string_view json
                        )
                         : _param_list(param_list)
                         , _json(json)
                        {}

                        template <typename F>
      bool              parse(F&& f);

   private:

      parameter_list    _param_list;
      std::string       _json;
   };

   namespace x3 = boost::spirit::x3;
   namespace fusion = boost::fusion;

   template <typename F>
   struct preset_callback
   {
      parameter const*     param;
      std::size_t const    size;
      std::size_t          index;
      F&&                  f;
   };

   template <typename F>
   inline preset_callback<F>
   make_preset_callback(preset::parameter_list params, F&& f)
   {
      return { params.begin(), params.size(), 0, std::forward<F>(f) };
   }

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

      // Main
      template <typename Iter, typename Ctx, typename F>
      bool parse_impl(Iter& first, Iter last, Ctx& context, preset_callback<F>& attr) const;
   };

   std::optional<std::string> extract_string(std::string_view in);

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

   inline std::optional<std::string> extract_string(std::string_view in)
   {
      std::string result;
      using uchar = std::uint32_t;   // a unicode code point
      auto i = in.begin();
      auto last = in.end();

      for (; i < last; ++i)
      {
         auto ch = *i;
         if (std::iscntrl(ch))
            return {};

         if (*i == '\\')
         {
            if (++i == last)
               return {};

            switch (ch = *i)
            {
               case 'b': result += '\b';     break;
               case 'f': result += '\f';     break;
               case 'n': result += '\n';     break;
               case 'r': result += '\r';     break;
               case 't': result += '\t';     break;
               default: result += ch;        break;

               case 'u':
               {
                  x3::uint_parser<uchar, 16, 4, 4> hex4;
                  uchar code_point;
                  auto ii = ++i;
                  bool r = x3::parse(ii, last, hex4, code_point);
                  if (!r)
                     return {};

                  i = ii; // update iterator position
                  using insert_iter = std::back_insert_iterator<std::string>;
                  insert_iter out_iter(result);
                  boost::utf8_output_iterator<insert_iter> utf8_iter(out_iter);
                  *utf8_iter++ = code_point;
               }
            }
         }
         else
         {
            result += ch;
         }
      }
      return result;
   }

   template <typename Iter, typename Ctx>
   inline bool
   parser::parse_impl(Iter& first, Iter last, Ctx& context, std::string_view& attr) const
   {
      x3::skip_over(first, last, context);

      // For strings, we do not decode nor validate anything. We simply
      // detect what looks like a double-quoted string which contains any
      // character, including any escaped double quote (i.e. "\\\""). It's up
      // to the client to decode the escapes. We provide a utility for that
      // (see extract_string).

      static auto double_quote = '"' >> *("\\\"" | (x3::char_ - '"')) >> '"';
      auto iter = first;
      if (double_quote.parse(iter, last, context, x3::unused, x3::unused))
      {
         // Don't include the start and end double-quotes
         attr = std::string_view{first+1, std::size_t(iter-first)-2};
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

   template <typename Iter, typename Ctx, typename F>
   inline bool
   parser::parse_impl(Iter& first, Iter last, Ctx& context, preset_callback<F>& attr) const
   {
      if (first == last)
         return false;

      if (attr.index == 0) // 0 signals start of parse
      {
         attr.index++;
         if (!x3::char_('{').parse(first, last, context, x3::unused, x3::unused))
            return false;
         while (parse_impl(first, last, context, attr))
         {
            if (!x3::char_(',').parse(first, last, context, x3::unused, x3::unused))
               break;
         }
         return x3::char_('}').parse(first, last, context, x3::unused, x3::unused);
      }

      auto& param = attr.param[attr.index-1]; // one based
      auto&& parse = [&, this](auto& pair)
      {
         bool r = parse_impl(first, last, context, pair);
         if (r)
            attr.f(pair, param, attr.index++ - 1);
         return r;
      };

      switch (param._type)
      {
         case parameter::bool_:
         {
            std::pair<std::string_view, bool> pair;
            return parse(pair);
         }
         break;
         case parameter::int_:
         {
            std::pair<std::string_view, int> pair;
            return parse(pair);
         }
         break;
         case parameter::frequency:
         case parameter::double_:
         {
            std::pair<std::string_view, double> pair;
            return parse(pair);
         }
         break;
         case parameter::note:
         {
            std::pair<std::string_view, std::string_view> pair;
            bool r = parse_impl(first, last, context, pair);
            if (r)
            {
               std::pair<std::string_view, std::uint8_t> note_pair;
               note_pair.first = pair.first;
               note_pair.second = q::midi::note_number(pair.second);
               attr.f(note_pair, param, attr.index++ - 1);
            }
            return r;
         }
         break;
      }
   }

   template <typename F>
   inline bool preset::parse(F&& f)
   {
      auto attr = make_preset_callback(_param_list, std::forward<F>(f));
      auto i = _json.begin();
      return x3::phrase_parse(i, _json.end(), parser{}, x3::space, attr);
   }
}}

#endif