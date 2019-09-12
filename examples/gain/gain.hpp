#pragma once

#include "IPlug_include_in_plug_hdr.h"

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

  void   ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
};
