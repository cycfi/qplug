#include "gain.hpp"
#include "IPlug_include_in_plug_src.h"

using namespace cycfi::elements;
using dial_ptr = std::shared_ptr<dial_base>;
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
      dial(radial_marks<20>(basic_knob<50>()))
   );

   auto markers = radial_labels<15>(
      hold(dial_),
      0.7,                                   // Label font size (relative size)
      "0", "1", "2", "3", "4",               // Labels
      "5", "6", "7", "8", "9", "10"
   );

   return align_center_middle(
      caption(markers, "Volume", 0.8)
   );
}

auto make_controls(dial_ptr& dial_)
{
   return min_size({ 150, 150 },
      align_center(align_middle(make_dial(dial_)))
   );
}

gain::gain(const InstanceInfo& info)
  : Plugin(info, MakeConfig(kNumParams, kNumPrograms))
{
    GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
}

void gain::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
   const double gain = GetParam(kGain)->Value() / 100.;
   const int nChans = NOutChansConnected();

   for (int s = 0; s < nFrames; s++)
   {
      for (int c = 0; c < nChans; c++)
      {
         outputs[c][s] = inputs[c][s] * gain;
      }
   }
}

void* gain::OpenWindow(void* pParent)
{
   _view = std::make_unique<view>(static_cast<cycfi::elements::host_view_handle>(pParent));

   _view->content(
      {
         share(make_controls(_dial)),
         share(background{})
      }
   );

   _dial->on_change =
      [&](double val)
      {
         SendParameterValueFromUI(kGain, val);
      };

  return _view->host();
}

void gain::CloseWindow()
{
   _view = nullptr;
}



