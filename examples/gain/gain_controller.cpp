/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "gain_controller.hpp"

using namespace elements;
auto constexpr bkd_color = rgba(35, 35, 37, 255);

struct background : element
{
   void draw(context const& ctx)
   {
      auto& cnv = ctx.canvas;
      cnv.fill_style(bkd_color);
      cnv.fill_rect(ctx.bounds);
   }
};

auto make_dial(dial_ptr& dial_)
{
   dial_ = share(
      dial(radial_marks<20>(basic_knob<65>()))
   );

   auto markers = radial_labels<20>(
      hold(dial_),
      0.9,                                   // Label font size (relative size)
      "0", "1", "2", "3", "4",               // Labels
      "5", "6", "7", "8", "9", "10"
   );

   return align_center_middle(
      caption(markers, "Volume", 0.9)
   );
}

auto make_controls(dial_ptr& dial_)
{
   return min_size({ 150, 150 },
      align_center(align_middle(make_dial(dial_)))
   );
}

gain_controller::gain_controller(base_controller& base)
 : qplug::controller(base)
{}

using parameter = qplug::parameter;
using parameter_list = gain_controller::parameter_list;

parameter_list gain_controller::parameters()
{
   static parameter params[] =
   {
      parameter{ "Gain", 100.0 }.range(0, 100).unit("%")
   };

   return { params };
}

void gain_controller::on_attach_view()
{
   view()->content(
      {
         share(make_controls(_dial)),
         share(background{})
      }
   );

   controls(_dial);
}





