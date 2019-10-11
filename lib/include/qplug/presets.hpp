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
   namespace x3 = boost::spirit::x3;
   namespace fusion = boost::fusion;
   using parameter_list = iterator_range<parameter const*>;
   using param_map = std::map<std::string_view, parameter const*>;

   template <typename F>
   struct preset_attr
   {
      F&&         preset_info;
      param_map   params;
   };

   template <typename F>
   inline preset_attr<F>
   on_preset(parameter_list params, F&& f)
   {
      preset_attr<F> r{std::forward<F>(f) };
      for (auto const& param : params)
         r.params[param._name] = &param;
      return std::move(r);
   }

   template <typename F1, typename F2>
   struct all_presets_attr
   {
      preset_attr<F1>   preset_callback;
      F2&&              preset_name;
   };

   template <typename F1, typename F2>
   inline all_presets_attr<F1, F2>
   for_each_preset(parameter_list params, F1&& f1, F2&& f2)
   {
      all_presets_attr<F1, F2> r{
         { std::forward<F1>(f1) }, std::forward<F2>(f2)
      };

      for (auto const& param : params)
         r.preset_callback.params[param._name] = &param;
      return std::move(r);
   }

   class preset_parser : public x3::parser<preset_parser>
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

      preset_parser const& self() const { return *this; }

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
      template <typename Iter, typename Ctx, typename F>
      bool parse_pair(Iter& first, Iter last, Ctx& context, preset_attr<F>& attr) const;

      // Preset
      template <typename Iter, typename Ctx, typename F>
      bool parse_impl(Iter& first, Iter last, Ctx& context, preset_attr<F>& attr) const;

      // Main
      template <typename Iter, typename Ctx, typename F1, typename F2>
      bool parse_impl(Iter& first, Iter last, Ctx& context, all_presets_attr<F1, F2>& attr) const;
   };

   std::optional<std::string> extract_string(std::string_view in);

///////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////
   template <typename Iter, typename Ctx, typename Attr>
   inline typename std::enable_if<std::is_floating_point<Attr>::value, bool>::type
   preset_parser::parse_impl(Iter& first, Iter last, Ctx& context, Attr& attr) const
   {
      static x3::real_parser<Attr> p;
      return p.parse(first, last, context, x3::unused, attr);
   }

   template <typename Iter, typename Ctx, typename Attr>
   inline typename std::enable_if<std::is_integral<Attr>::value, bool>::type
   preset_parser::parse_impl(Iter& first, Iter last, Ctx& context, Attr& attr) const
   {
      static x3::int_parser<Attr> p;
      return p.parse(first, last, context, x3::unused, attr);
   }

   template <typename Iter, typename Ctx>
   inline bool
   preset_parser::parse_impl(Iter& first, Iter last, Ctx& context, bool& attr) const
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
   preset_parser::parse_impl(Iter& first, Iter last, Ctx& context, std::string_view& attr) const
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

   template <typename Iter, typename Ctx, typename F>
   inline bool
   preset_parser::parse_pair(Iter& first, Iter last, Ctx& context, preset_attr<F>& attr) const
   {
      if (first == last)
         return false;

      auto&& parse = [&](auto const& p)
      {
         return p.parse(first, last, context, x3::unused, x3::unused);
      };

      std::string_view name;
      if (parse_impl(first, last, context, name))
      {
         if (!parse(x3::char_(':')))
            return false;

         auto i = attr.params.find(name);
         if (i == attr.params.end())
            return false;

         auto& param = *i->second;

         auto&& parse_attr = [&, this](auto& val)
         {
            bool r = parse_impl(first, last, context, val);
            if (r)
               attr.preset_info(std::make_pair(name, val), param);
            return r;
         };

         auto&& parse_transform = [&, this](auto& val, auto&& transform)
         {
            bool r = parse_impl(first, last, context, val);
            if (r)
               attr.preset_info(std::make_pair(name, transform(val)), param);
            return r;
         };

         switch (param._type)
         {
            case parameter::bool_:
            {
               bool attr;
               return parse_attr(attr);
            }
            case parameter::int_:
            {
               int attr;
               return parse_attr(attr);
            }
            case parameter::frequency:
            case parameter::double_:
            {
               double attr;
               return parse_attr(attr);
            }
            case parameter::note:
            {
               std::string_view attr;
               return parse_transform(attr
                , [](auto& attr) { return q::midi::note_number(attr); }
               );
            }
         }
      }
      return false;
   }

   template <typename Iter, typename Ctx, typename F>
   inline bool
   preset_parser::parse_impl(Iter& first, Iter last, Ctx& context, preset_attr<F>& attr) const
   {
      if (first == last)
         return false;

      auto&& parse = [&](auto const& p)
      {
         return p.parse(first, last, context, x3::unused, x3::unused);
      };

      if (!parse(x3::char_('{')))
         return false;

      while (parse_pair(first, last, context, attr))
      {
         if (!parse(x3::char_(',')))
            break;
      }

      return parse(x3::char_('}'));
   }

   template <typename Iter, typename Ctx, typename F1, typename F2>
   inline bool
   preset_parser::parse_impl(Iter& first, Iter last, Ctx& context, all_presets_attr<F1, F2>& attr) const
   {
      if (first == last)
         return false;

      auto&& parse = [&](auto const& p)
      {
         return p.parse(first, last, context, x3::unused, x3::unused);
      };

      if (!parse(x3::char_('{')))
         return false;

      std::string_view name;
      while (parse_impl(first, last, context, name))
      {
         if (!parse(x3::char_(':')))
            return false;
         attr.preset_name(name);

         if (parse_impl(first, last, context, attr.preset_callback))
         {
            if (parse(x3::char_(',')))
               continue;
         }
      }

      return parse(x3::char_('}'));
   }
}}

#endif