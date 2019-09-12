#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include <memory>
#include <elements.hpp>

const int kNumPrograms = 1;

enum EParams
{
   kGain = 0,
   kNumParams
};

using namespace iplug;
using namespace igraphics;

class gain : public Plugin
{
public:

   gain(const InstanceInfo& info);

   void     ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
   void*    OpenWindow(void* pParent) override;
   void     CloseWindow() override;

private:

   using view = cycfi::elements::view;
   using dial_ptr = std::shared_ptr<cycfi::elements::dial_base>;

   dial_ptr _dial;
   std::unique_ptr<view> _view;
};
